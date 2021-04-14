#define _GLIBCXX_USE_NANOSLEEP    // Workaround to use std::this_thread::sleep_for

#include <inttypes.h>
#include <iostream>
#include <iomanip>
#include <yaml-cpp/yaml.h>
#include <arpa/inet.h>
#include <thread>
#include <sys/mman.h>
#include <boost/atomic.hpp>
#include <cpsw_api_user.h>
#include "heartbeat.h"

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
    Tester( Path root, uint32_t timeout, size_t timerBufferSize );
    ~Tester();

private:
    ScalVal             enable;
    ScalVal             swEnable;
    Stream              strm0;
    Stream              strm1;
    Command             swErrClr;
    HeartBeat           hb;
    Timer<double>       rxT;
    boost::atomic<bool> run;
    std::thread         rxThreadMain;
    std::thread         rxThreadSecondary;

    void rxHandlerMain();
    void rxHandlerSecondary();
};

Tester::Tester( Path root, uint32_t timeout, size_t timerBufferSize )
:
enable            ( IScalVal::create( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/Enable" ) ) ),
swEnable          ( IScalVal::create( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareEnable" ) ) ),
strm0             ( IStream::create ( root->findByName( "/Stream0" ) ) ),
strm1             ( IStream::create ( root->findByName( "/Stream1" ) ) ),
swErrClr          ( ICommand::create( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SwErrClear" ) ) ),
hb                ( root, timeout, timerBufferSize ),
rxT               ( "Rx Periods", timerBufferSize ),
run               ( true ),
rxThreadMain      ( std::thread( &Tester::rxHandlerMain, this ) ),
rxThreadSecondary ( std::thread( &Tester::rxHandlerSecondary, this ) )
{
    std::cout << "Creating Tester object..." << std::endl;

    if ( pthread_setname_np( rxThreadMain.native_handle(), "rxThreadMain" ) )
      perror( "pthread_setname_np failed for rxThreadMain" );

    if ( pthread_setname_np( rxThreadSecondary.native_handle(), "rxThreadSecond" ) )
      perror( "pthread_setname_np failed for rxThreadSecondary" );

    std::cout << "Tester created." << std::endl;
    std::cout << std::endl;
}


Tester::~Tester()
{
    std::cout << "Destroying Tester object..." << std::endl;

    std::cout << "Stopping the Rx Thread..." << std::endl;
    run = false;
    rxThreadMain.join();
    rxThreadSecondary.join();

    std::cout << "Disabling MPS FW Software engine..." << std::endl;
    swEnable->setVal( static_cast<uint64_t>(0) );
    enable->setVal( static_cast<uint64_t>(0) );

    std::cout << "Tester destroyed." << std::endl;
    std::cout << std::endl;
    std::cout << std::flush;
}

void Tester::rxHandlerMain()
{
    std::cout << "Rx main thread started" << std::endl;
    std::cout << std::endl;

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

    uint8_t     buf[100*1024];
    int         rxCounter       { 0 };     // Number of packet received
    int         toCounter       { 0 };     // Number of RX timeouts

    std::cout << "Enabling MPS FW Software engine..." << std::endl;
    enable->setVal( static_cast<uint64_t>(1) );
    swEnable->setVal( static_cast<uint64_t>(1) );

    // Wait for the first packet, for 100ms. If we timeout here, we abort the test
    if ( 0 == strm0->read(buf, 102400, CTimeout(100000)) )
    {
        std::cerr << "We didn't receive the first stream packet after 100ms. Aborting test" << std::endl;
        exit(-3);
    }

    // Clear the software error flags before starting
    swErrClr->execute();

    // Reset all heart beat counters
    hb.beat();
    hb.clear();

    // Start the receiver timer
    rxT.start();

    while(run)
    {
        if (0 != strm0->read(buf, sizeof(buf), CTimeout(3500)))
        {
            rxT.tick();
            ++rxCounter;
            hb.beat();
        }
        else
        {
            ++toCounter;
            rxT.start();
        }

        // After 2 heartbeats have been sent, we need to clear the software
        // errors in order to reset the SoftwareHbUpCntMax register.
        if (2 == rxCounter)
            swErrClr->execute();
    }

    std::cout << "Rx main thread interrupted" << std::endl;
    std::cout << std::endl;

    std::cout << "Rx main thread report:" << std::endl;
    std::cout << "===========================" << std::endl;
    std::cout << "Rx packet counter  = " << rxCounter << std::endl;
    std::cout << "Rx timeout counter = " << toCounter << std::endl;
    rxT.show();
    std::cout << std::endl;

    std::cout << std::flush;
}

void Tester::rxHandlerSecondary()
{
    std::cout << "Rx secondary thread started" << std::endl;

    // Declare as real time task
    struct sched_param  param;
    param.sched_priority = rtPriority - 1;
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

    std::size_t rxPackets      { 0 }; // Number of packet received
    uint8_t     buf[100*1024];        // 100KBytes buffer

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
    std::cout << "This program starts the FW engine and receives the update packets comming from FW." << std::endl;
    std::cout << "Each time a packet is received, a heartbeat is send to the FW." << std::endl;
    std::cout << "The program runs for the specified number of seconds, printing a report at the end." << std::endl;
    std::cout << "Usege: " << name << " -a <IP_address> -Y <Yaml_top> -s <Seconds to run the test> [-T <Watchdog timeout>] [-h]" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char **argv)
{
    int           c;
    int           seconds = 0;
    unsigned char buf[sizeof(struct in6_addr)];
    std::string   ipAddr;
    std::string   yamlDoc;
    uint32_t      timeout = 3500;

    while((c =  getopt(argc, argv, "a:Y:s:T:h")) != -1)
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
            case 's':
                if (1 != sscanf(optarg, "%d", &seconds))
                {
                    printf("Invalid number of periods\n");
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
                usage( argv[0] );
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

    if (seconds <= 0)
    {
        std::cout << "Number of seconds must be greater that 0" << std::endl;
        exit(1);
    }

    std::cout << "Loading YAML description..." << std::endl;
    IYamlSetIP setIP(ipAddr);
    Path root = IPath::loadYamlFile(yamlDoc.c_str(), "NetIODev", NULL, &setIP);

    // Create CPSW interfaces for AxiVersion registers
    ScalVal_RO fpgaVers = IScalVal_RO::create (root->findByName("/mmio/AmcCarrierCore/AxiVersion/FpgaVersion"));
    ScalVal_RO bldStamp = IScalVal_RO::create (root->findByName("/mmio/AmcCarrierCore/AxiVersion/BuildStamp"));
    ScalVal_RO gitHash  = IScalVal_RO::create (root->findByName("/mmio/AmcCarrierCore/AxiVersion/GitHash"));

    // Print Version Information
    std::cout << std::endl;
    std::cout << "Version Information:" << std::endl;

    uint64_t u64;
    fpgaVers->getVal(&u64);
    std::cout << "FPGA Version : " << u64 << std::endl;

    uint8_t str[256];
    bldStamp->getVal(str, sizeof(str)/sizeof(str[0]));
    std::cout << "Build String : " << str << std::endl;

    uint8_t hash[20];
    gitHash->getVal(hash, 20);
    std::cout << "Git hash     : ";
    std::ostringstream aux;
    for (int i = 19; i >= 0; --i)
        aux << std::hex << std::setfill('0') << std::setw(2) << unsigned(hash[i]);
    std::string gitHashStr(aux.str());
    std::cout << gitHashStr << std::endl;

    // Start test
    Tester t( root, timeout, seconds*360 );

    // Now wait for the defined time
    std::cout << "Now waiting for " << seconds << " seconds..."<< std::endl;
    std::cout << std::endl;

    std::this_thread::sleep_for( std::chrono::seconds( seconds ) );

    std::cout << "End of program." << std::endl;

    return 0;
}
