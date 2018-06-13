#define _GLIBCXX_USE_NANOSLEEP    // Workaround to use std::this_thread::sleep_for

#include <inttypes.h>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <boost/atomic.hpp>
#include <cpsw_api_user.h>
#include "heartbeat.h"

boost::atomic<int> counter(0);

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
    Tester( Path root, uint32_t timeout, size_t timerBufferSize );
    ~Tester();

private:
    ScalVal             enable;
    boost::atomic<bool> run;
    HeartBeat           hb;
    std::thread         workerThread;

    void worker();
};

Tester::Tester( Path root, uint32_t timeout, size_t timerBufferSize )
:
enable       ( IScalVal::create( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/Enable" ) ) ),
run          ( true ),
hb           ( root, timeout, timerBufferSize ),
workerThread ( std::thread( &Tester::worker, this ) )
{
    std::cout << "Creating Tester object..." << std::endl;

    if ( pthread_setname_np( workerThread.native_handle(), "workerThread" ) )
      perror( "pthread_setname_np failed for workerThread" );

    std::cout << "Enabling MPS FW  engine..." << std::endl;
    enable->setVal( static_cast<uint64_t>( 1 ) );

    std::cout << "Tester created." << std::endl;
    std::cout << std::endl;
}


Tester::~Tester()
{
    std::cout << "Destroying Tester object..." << std::endl;

    std::cout << "Stopping the Rx Thread..." << std::endl;
    run = false;
    workerThread.join();

    std::cout << "Disabling MPS FW Software engine..." << std::endl;
    enable->setVal( static_cast<uint64_t>(0) );

    std::cout << "Tester destroyed." << std::endl;
    std::cout << std::endl;
}

void Tester::worker()
{
    std::cout << "Worker Thread started" << std::endl;
    std::cout << std::endl;

    struct sched_param  param;
    param.sched_priority = 20;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("Set priority");
        std::cerr << "WARN: Setting thread RT priority failed on Heartbeat thread." << std::endl;
    }

    while(run)
    {
        std::this_thread::sleep_for( std::chrono::microseconds( 2777 ) );
        hb.beat();
        ++counter;
    }

    std::cout << "Worker Thread interrupted" << std::endl;
    std::cout << std::endl;
}

void usage(char* name)
{
    std::cout << "This program starts the FW engine and send heartbeat each 2.7ms aprox." << std::endl;
    std::cout << "The program runs for the specified number of seconds aprox., printing a report at the end." << std::endl;
    std::cout << "Usege: " << name << " -a <IP_address> -Y <Yaml_top> -s <Seconds to run the test> [-T <Watchdog timeout>]i [-h]" << std::endl;
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

    if (seconds <= 0)
    {
        std::cout << "Number of seconds must be greater that 0" << std::endl;
        exit(1);
    }

    printf("Loading YAML description...\n");
    IYamlSetIP setIP(ipAddr);
    Path root = IPath::loadYamlFile(yamlDoc.c_str(), "NetIODev", NULL, &setIP);

    Tester t( root, timeout, seconds*360-1 ); // Discard the first point in the timer buffers

    // Now wait for the defined time
    std::cout << "Now waiting for " << seconds << " seconds..."<< std::endl;
    std::cout << std::endl;

    // Wait for 'secodns' equivalent number of iterations 
    while ( counter < seconds*360-1 )
      std::this_thread::sleep_for( std::chrono::microseconds( 500 ) );

    std::cout << "End of program." << std::endl;

    return 0;
}
