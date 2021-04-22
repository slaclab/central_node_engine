#include <inttypes.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <arpa/inet.h>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <thread>
#include <sys/mman.h>
#include <cpsw_api_user.h>
#include <boost/shared_ptr.hpp>
#include <boost/atomic.hpp>
#include "timer.h"

const int rtPriority   = 49;        /* we use 49 as the PRREMPT_RT use 50
                                       as the priority of kernel tasklets
                                       and interrupt handler by default */

const int maxSafeStack = 8*1024;    /* The maximum stack size which is
                                       guaranteed safe to access without
                                       faulting */

// Function and Class definitions
void stack_prefault(void)
{
    unsigned char dummy[maxSafeStack];

    memset(dummy, 0, maxSafeStack);
    return;
}

class IYamlSetIP : public IYamlFixup
{
    public:
        IYamlSetIP( std::string ip_addr ) : ip_addr_(ip_addr) {}

        virtual void operator()(YAML::Node &root, YAML::Node &top)
        {
            YAML::Node ipAddrNode = IYamlFixup::findByName(root, "ipAddr");

            if ( ! ipAddrNode )
                throw std::runtime_error("ERROR on IYamlSetIP::operator(): 'ipAddr' node was not found!");

            ipAddrNode = ip_addr_.c_str();
        }

        ~IYamlSetIP() {}

    private:
        std::string ip_addr_;
};

class Tester
{
public:
    Tester( Path root, size_t iterations, uint32_t timeout );
    ~Tester();

private:
    ScalVal                      enable;
    ScalVal                      swEnable;
    ScalVal                      swWdTime;
    ScalVal_RO                   swWdError;
    ScalVal                      swHeartBeat;
    Stream                       strm0;
    Stream                       strm1;
    std::vector< Timer<double> > t1;
    std::vector<int>             c1;
    boost::atomic<bool>          run;
    std::thread                  rxThreadMain;
    std::thread                  rxThreadSecondary;

    void rxHandlerMain();
    void rxHandlerSecondary();

    static void printTimerVal(Timer<double>& t) { std::cout << std::setw(8) << t.getMinPeriod()*1e6 << ", "; };
    static void printCounterVal(int c)          { std::cout << std::setw(8) << c << ", ";                    };
};

Tester::Tester( Path root, size_t iterations, uint32_t timeout )
:
    enable      ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/Enable" ) ) ),
    swEnable    ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareEnable" ) ) ),
    swWdTime    ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdTime" ) ) ),
    swWdError   ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdError" ) ) ),
    swHeartBeat ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdHeartbeat" ) ) ),
    strm0       ( IStream::create ( root->findByName( "/Stream0" ) ) ),
    strm1       ( IStream::create ( root->findByName( "/Stream1" ) ) ),
    t1          ( iterations, Timer<double>("Waiting timeout") ),
    c1          ( iterations, 1 ),
    run               ( true ),
    rxThreadMain      ( std::thread( &Tester::rxHandlerMain, this ) ),
    rxThreadSecondary ( std::thread( &Tester::rxHandlerSecondary, this ) )
{
    if ( pthread_setname_np( rxThreadMain.native_handle(), "rxThreadMain" ) )
      perror( "pthread_setname_np failed for rxThreadMain" );

    if ( pthread_setname_np( rxThreadSecondary.native_handle(), "rxThreadSecond" ) )
      perror( "pthread_setname_np failed for rxThreadSecondary" );

    // Declare as real time task
    struct sched_param  param;
    param.sched_priority = rtPriority;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("sched_setscheduler failed");
        exit(-1);
    }

    // Lock memory
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1)
    {
        perror("mlockall failed");
        exit(-2);
    }

    // Pre-fault our stack
    stack_prefault();

    std::cout << "Starting the test..." << std::endl;

    std::cout << "Setting timeout to " << timeout << " us" << std::endl;
    swWdTime->setVal( timeout );

    std::cout << "Enabling the MPS engine" << std::endl;
    enable->setVal( static_cast<uint64_t>( 1 ) );
    swEnable->setVal( static_cast<uint64_t>( 1 ) );

    std::cout << "Running iterations..." << std::endl;
    uint32_t u32;

    for( size_t i(0); i<iterations; ++i )
    {
        // Send heartbeat command (in FW is active on the risig edge)
        swHeartBeat->setVal( static_cast<uint64_t>( 1 ) );

        // Start measuring the time until timeout
        t1.at(i).start();

        // Wait for the watchdog error flag to be set
        swWdError->getVal( &u32 );
        while(0 == u32)
        {
            swWdError->getVal( &u32 );
            ++c1.at(i);
        }

        // End of the timer
        t1.at(i).tick();

        // Clear the heartbeat flag (this is a falling edge which has no effect on FW)
        swHeartBeat->setVal( static_cast<uint64_t>( 0 ) );
    }

   std::cout << "End of iterations." << std::endl;

}


Tester::~Tester()
{
    run = false;
    rxThreadMain.join();
    rxThreadSecondary.join();

    std::cout << "Stoping MPS engine" << std::endl;
    enable->setVal( static_cast<uint64_t>( 0 ) );
    swEnable->setVal( static_cast<uint64_t>( 0 ) );

    std::cout << "Measure report:" << std::endl;
    std::cout << "=============================" << std::endl;

    std::cout << "Time to timeout (us)                    = ";
    std::for_each( t1.begin(), t1.end(), printTimerVal );
    std::cout << std::endl;
    //std::for_each(t1.begin(), t1.end(), std::mem_fun_ref(&Timer<double>::show));
    //
    std::cout << "Number of register polls (counts)       = ";
    //std::copy( c1.begin(), c1.end(), std::ostream_iterator<int>(std::cout, ", "));
    std::for_each( c1.begin(), c1.end(), printCounterVal );
    std::cout << std::endl;

    std::cout << "=============================" << std::endl;
}

void Tester::rxHandlerMain()
{
    std::cout << "Rx main thread started" << std::endl;

    std::size_t rxPackets { 0 }; // Number of packet received
    uint8_t     buf[100*1024];   // 100KBytes buffer

    while(run)
    {
        if(strm0->read(buf, sizeof(buf), CTimeout(3500)))
            // Increase RX packet counter
            ++rxPackets;
    }

    std::cout << "Rx main thread interrupted" << std::endl;
    std::cout << std::endl;

    std::cout << "Rx main thread report:" << std::endl;
    std::cout << "===========================" << std::endl;
    std::cout << "Number of packet received : " << rxPackets << std::endl;
    std::cout << std::endl;

    std::cout << std::flush;
}

void Tester::rxHandlerSecondary()
{
    std::cout << "Rx secondary thread started" << std::endl;

    std::size_t rxPackets { 0 }; // Number of packet received
    uint8_t     buf[100*1024];   // 100KBytes buffer

    while(run)
    {
        if(strm1->read(buf, sizeof(buf), CTimeout(3500)))
            // Increase RX packet counter
            ++rxPackets;
    }

    std::cout << "Rx secondary thread interrupted" << std::endl;
    std::cout << std::endl;

    std::cout << "Rx secondary thread report:" << std::endl;
    std::cout << "===========================" << std::endl;
    std::cout << "Number of packet received : " << rxPackets << std::endl;
    std::cout << std::endl;

    std::cout << std::flush;
}

void usage(char* name)
{
    std::cout << "This program measures the the watch dog take to timeouts, how how much time the error flag remains set after sending the heartbeat." << std::endl;
    std::cout << "The program runs for the specified number of iterations printing a report at the end." << std::endl;
    std::cout << "Usege: " << name << " -a <IP_address> -Y <Yaml_top> -i <iterations to run the test> [-T <Watchdog timeout>] [-h]" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char **argv)
{
    int           c;
    size_t        iterations = 0;
    unsigned char buf[sizeof(struct in6_addr)];
    std::string   ipAddr;
    std::string   yamlDoc;
    uint32_t      timeout = 3500;

    while((c =  getopt(argc, argv, "a:Y:i:T:h")) != -1)
    {
        switch (c)
        {
            case 'a':
                if (!inet_pton(AF_INET, optarg, buf))
                {
                    std::cout << "Invalid IP address..." << std::endl;
                    exit(1);
                }
                ipAddr = std::string(optarg);
                break;
            case 'Y':
                yamlDoc = std::string(optarg);
                break;
            case 'i':
                if (1 != sscanf(optarg, "%zu", &iterations))
                {
                    printf("Invalid number of iterations\n");
                    exit(1);
                }
                break;
            case 'T':
                uint32_t aux;
                if (1 != sscanf(optarg, "%d", &aux))
                {
                    printf("Invalid timeout value\n");
                    exit(1);
                }
                timeout = aux;
                break;
            case 'h':
                usage( argv[0] );
                exit(0);
                break;
            default:
                std::cout << "Invalid option." << std::endl;
                std::cout << std::endl;
                usage( argv[0]  );
                exit(1);
                break;
        }
    }

    if (ipAddr.empty())
    {
        std::cout << "Must specify an IP address." << std::endl;
        exit(1);
    }

    if (yamlDoc.empty())
    {
        std::cout << "Must specify a Yaml top path." << std::endl;
        exit(1);
    }

    if (iterations == 0)
    {
        std::cout << "Number of iterations must be greater that 0" << std::endl;
        exit(1);
    }

    printf("Loading YAML description...\n");
    IYamlSetIP setIP(ipAddr);
    Path root = IPath::loadYamlFile(yamlDoc.c_str(), "NetIODev", NULL, &setIP);

    Tester t( root, iterations, timeout );

    std::cout << "End of program." << std::endl;

    return 0;
}
