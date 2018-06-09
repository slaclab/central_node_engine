#define _GLIBCXX_USE_NANOSLEEP    // Workaround to use std::this_thread::sleep_for

#include <inttypes.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <arpa/inet.h>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cpsw_api_user.h>
#include <boost/shared_ptr.hpp>
#include "timer.h"

class IYamlSetIP : public IYamlFixup
{
    public:
        IYamlSetIP( std::string ip_addr ) : ip_addr_(ip_addr) {}

        void operator()(YAML::Node &node)
        {
            node["ipAddr"] = ip_addr_.c_str();
        }

        ~IYamlSetIP() {}

    private:
        std::string ip_addr_;
};

class Tester
{
public:
    Tester( Path root, size_t iterations, uint32_t timeouti, bool wait );
    ~Tester();

private:
    ScalVal       enable;
    ScalVal       swEnable;
    ScalVal       swWdTime;
    ScalVal_RO    swWdError;
    ScalVal       swHeartBeat;
    Timer<double> t1;
    Timer<double> t2;
    int           c;
    bool          wait;
};

Tester::Tester( Path root, size_t iterations, uint32_t timeout, bool wait )
:
    enable      ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/Enable" ) ) ),
    swEnable    ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareEnable" ) ) ),
    swWdTime    ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdTime" ) ) ),
    swWdError   ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdError" ) ) ),
    swHeartBeat ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdHeartbeat" ) ) ),
    t1          ( "Waiting timeout", iterations ),
    t2          ( "2.7ms delay", iterations ),
    c           ( 0 ),
    wait        ( wait )
{
    // Declare as real-time task
    struct sched_param  param;
    param.sched_priority = 20;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("Set priority");
        std::cerr << "WARN: Setting thread RT priority failed on Heartbeat thread." << std::endl;
    }

    std::cout << "Starting the test..." << std::endl;

    std::cout << "Setting timeout to " << timeout << " us" << std::endl;
    swWdTime->setVal( timeout );

    std::cout << "Enabling the MPS engine" << std::endl;
    enable->setVal( static_cast<uint64_t>( 1 ) );
    swEnable->setVal( static_cast<uint64_t>( 1 ) );

    std::cout << "Running iterations..." << std::endl;
    uint32_t u32;

    swHeartBeat->setVal( static_cast<uint64_t>( 0 ) );

    for( size_t i(0); i<iterations; ++i )
    {
        // Send heartbeat command (in FW is active on the risig edge)
        swHeartBeat->setVal( static_cast<uint64_t>( 1 ) );

        // Start measuring the time to timeout
        t1.start();

        // Start the 2.7ms delay, and measure it
        t2.start();
        std::this_thread::sleep_for( std::chrono::microseconds( 2777 ) );
        t2.tick();
     
        // Check the watchdog error flag. If set it will increase the counter   
        swWdError->getVal( &u32 );
        c += u32;

        // If we are waiting for the time to timeout poll the register until the flag is set
        // and measure the time
        if ( wait )
        {
            while( 0 == u32 )
                swWdError->getVal( &u32 );

            t1.tick();
        }
 
        // Clear the heartbeat flag (this is a falling edge which has no effect on FW)
        swHeartBeat->setVal( static_cast<uint64_t>( 0 ) );
    }

   std::cout << "End of iterations." << std::endl;

} 


Tester::~Tester()
{
    std::cout << "Stoping MPS engine" << std::endl;
    enable->setVal( static_cast<uint64_t>( 0 ) );
    swEnable->setVal( static_cast<uint64_t>( 0 ) );

    std::cout << "Measure report:" << std::endl;
    std::cout << "=============================" << std::endl;
    if ( wait )
        t1.show();
    t2.show();
    std::cout << "Sw errors after the 2.7ms delay = " << c << std::endl;
    std::cout << "=============================" << std::endl;
}

void usage(char* name)
{
    std::cout << "This program sends a heartbeat and wait for 2.7ms. It then checks if the software error flag was set right after the delay and accumulate it." << std::endl;
    std::cout << "If not, it keeps waiting for timeout flag to be set, measuring the time." << std::endl;
    std::cout << "The program runs for the specified number of iterations printing a report at the end." << std::endl;
    std::cout << "Usege: " << name << " -a <IP_address> -Y <Yaml_top> -i <iterations to run the test> [-n <do not wait for timeout after 2.7ms delay>] [-T <Watchdog timeout>] [-h]" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char **argv)
{
    int           c;
    unsigned char buf[sizeof(struct in6_addr)];
    std::string   ipAddr;
    std::string   yamlDoc;
    size_t        iterations = 0;
    uint32_t      timeout    = 3500;
    bool          wait       = true;

    while((c =  getopt(argc, argv, "a:Y:i:T:nh")) != -1)
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
            case 'n':
                wait = false;
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

    if (iterations <= 0)
    {
        std::cout << "Number of iterations must be greater that 0" << std::endl;
        exit(1);
    }

    printf("Loading YAML description...\n");
    IYamlSetIP setIP(ipAddr);
    Path root = IPath::loadYamlFile(yamlDoc.c_str(), "NetIODev", NULL, &setIP);

    Tester t( root, iterations, timeout, wait );

    std::cout << "End of program." << std::endl;

    return 0;
}
