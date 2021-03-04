#define _GLIBCXX_USE_NANOSLEEP    // Workaround to use std::this_thread::sleep_for

#include <iostream>
#include <map>
#include <iterator>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <signal.h>
#include <sys/mman.h>
#include <boost/atomic.hpp>
#include <chrono>
#include <arpa/inet.h>
#include <yaml-cpp/yaml.h>
#include <cpsw_api_user.h>

// Constant definitions
const int fpgaClkPerUs = 250;       // FPGA clock per microsecond

const int rtPriority   = 49;        /* we use 49 as the PRREMPT_RT use 50
                                       as the priority of kernel tasklets
                                       and interrupt handler by default */

const int maxSafeStack = 8*1024;    /* The maximum stack size which is
                                       guaranteed safe to access without
                                       faulting */

// Function and Class defintions
void stack_prefault(void)
{
    unsigned char dummy[maxSafeStack];

    memset(dummy, 0, maxSafeStack);
    return;
}

void intHandler(int dummy) {}

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

class RAIIFile
{
public:
    RAIIFile(const std::string &name) : fileName(name), file(name, std::ofstream::out | std::ofstream::binary)
    {
        if (!file.is_open())
            throw std::ofstream::failure("Could not open " + fileName);
    };
    ~RAIIFile() { file.close(); };

    const std::string& getName() { return fileName; };

    template <typename T>
    RAIIFile& operator<< (const T& rhs)
    {
        file << rhs;
        return *this;
    };

    template<typename T, typename U>
    void writePair(const std::pair<T, U>& rhs)
    {
        file << rhs.first << "               " << rhs.second << "\n";
    };

private:
    std::string     fileName;
    std::ofstream   file;
};

class Tester
{
public:
    Tester(Path root, const std::string &fwGitHash, const std::string& outputDir);
    ~Tester();

    void rxHandler();

private:
    ScalVal             enable;
    ScalVal             swEnable;
    ScalVal             swClear;
    ScalVal_RO          txClkCnt;
    ScalVal_RO          swLossError;
    ScalVal_RO          swLossCnt;
    Stream              strm;
    std::string         gitHash;
    std::string         outDir;
    RAIIFile            outFile;
    boost::atomic<bool> run;
    std::thread         rxThread;
};

Tester::Tester(Path root, const std::string &fwGitHash, const std::string& outputDir)
:
enable          ( IScalVal::create( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/Enable" ) ) ),
swEnable        ( IScalVal::create( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareEnable" ) ) ),
swClear         ( IScalVal::create( root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareClear") ) ),
txClkCnt        ( IScalVal_RO::create( root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareBwidthCnt") ) ),
swLossError     ( IScalVal_RO::create (root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareLossError") ) ),
swLossCnt       ( IScalVal_RO::create (root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareLossCnt") ) ),
strm            ( IStream::create( root->findByName("/Stream0") ) ),
gitHash         ( fwGitHash ),
outDir          ( outputDir ),
outFile         ( outDir + "/" + gitHash.substr(0, 7) + ".data" ),
run             ( true ),
rxThread        ( std::thread( &Tester::rxHandler, this ) )
{
    std::cout << "Creating Tester object..." << std::endl;

    if ( pthread_setname_np( rxThread.native_handle(), "rxThread" ) )
      perror( "pthread_setname_np failed for rxThread" );

    std::cout << "Enabling MPS FW Software engine..." << std::endl;
    enable->setVal( static_cast<uint64_t>(1) );
    swEnable->setVal( static_cast<uint64_t>(1) );

    std::cout << "Tester created." << std::endl;
    std::cout << std::endl;
}

Tester::~Tester()
{
    std::cout << "Destroying Tester object..." << std::endl;

    std::cout << "Stopping the Rx Thread..." << std::endl;
    run = false;
    rxThread.join();

    std::cout << "Disabling MPS FW Software engine..." << std::endl;
    swEnable->setVal( static_cast<uint64_t>(0) );
    enable->setVal( static_cast<uint64_t>(0) );

    std::cout << "Tester destroyed." << std::endl;
    std::cout << std::endl;
}

void Tester::rxHandler()
{
    std::cout << "Rx Thread started" << std::endl;

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

    std::size_t                     rxPackages = 0; // Number of packet received
    int                             rxTimeouts = 0; // Number of RX timeouts
    std::map<uint32_t, std::size_t> h;              // Histogram (rxTime(us))
    int64_t                         got;
    uint32_t                        u32;
    uint8_t                         buf[100*1024];  // 100KBytes buffer

    // Wait for the first package
    if ( ! strm->read(buf, sizeof(buf), CTimeout(3500)) )
        ++rxTimeouts;

    // Clear the software error flags before starting
    swClear->setVal((uint64_t)1);
    swClear->setVal((uint64_t)0);

    while(run)
    {
        got = strm->read(buf, sizeof(buf), CTimeout(3500));
        if ( ! got )
        {
            ++rxTimeouts;
        }
        else
        {
      	    // Number of FPGA clock it took the current TX
       	    txClkCnt->getVal(&u32);

            // Round the time to us and update the histogram
            ++h[u32/fpgaClkPerUs];

            // Increase RX packet counter
            ++rxPackages;
        }

    }

    std::cout << "Rx Thread interrupted" << std::endl;
    std::cout << std::endl;

    uint8_t  packetLossError;
    uint32_t packetLossCnt;

    std::cout << "Rx Thread report:" << std::endl;
    std::cout << "===========================" << std::endl;
    std::cout << "Number of packet received : " << rxPackages << std::endl;

    // Do not print this info if not package was received
    if (rxPackages)
    {
        std::cout << "Number of timeouts        : " << rxTimeouts << std::endl;
        std::cout << "Min RX time (us)          : " << h.begin()->first << std::endl;
        std::cout << "Max RX time (us)          : " << h.rbegin()->first << std::endl;
    }

    swLossError->getVal(&packetLossError);
    std::cout << "FW SoftwareLossError      : " << unsigned(packetLossError) << std::endl;
    swLossCnt->getVal(&packetLossCnt);
    std::cout << "FW SoftwareLossCnt        : " << unsigned(packetLossCnt) << std::endl;

    // Do not create the data file is not package was received
    if (rxPackages)
    {
        std::cout << "Writing data to           : '" << outFile.getName() << "' ... ";

        // Write the histogram result to the output file
        outFile << "# FW version                : " << gitHash              << "\n";
        outFile << "# FW version                : " << gitHash.c_str()      << "\n";
        outFile << "# Number of packet received : " << rxPackages           << "\n";
        outFile << "# Number of timeouts        : " << rxTimeouts           << "\n";
        outFile << "# Min RX time (us)          : " << h.begin()->first     << "\n";
        outFile << "# Max RX time (us)          : " << h.rbegin()->first    << "\n";
        outFile << "# FW SoftwareLossCnt        : " << packetLossCnt        << "\n";
        outFile << "#\n";
        outFile << "# RxTime (us)     Counts\n";
        std::for_each(h.begin(), h.end(), std::bind(&RAIIFile::writePair<uint32_t, std::size_t>, &outFile, std::placeholders::_1));
    }
    else
    {
        std::cout << "Data file not created, as no package was received." << std::endl;
    }

    std::cout << "done!" << std::endl;

    std::cout << std::endl;
}

void usage(char* name)
{
    std::cout << "This program collects the time it takes the FW to send the update messages to the software." << std::endl;
    std::cout << "The program runs for the specified number of seconds printing a report at the end." << std::endl;
    std::cout << "The program also generates an histogram which is written to a file in an specified directory." << std::endl;
    std::cout << "The output historgram file name will the firmware short githash (7 chars) plus a \".data\" extention." << std::endl;
    std::cout << "Usege: " << name << " -a <IP_address> -Y <Yaml_top> -s <seconds to run the test> -d <output directory> [-h]" << std::endl;
    std::cout << std::endl;
}

// Main body
int main(int argc, char **argv)
{
    int           c;
    size_t        seconds = 0;
    unsigned char buf[sizeof(struct in6_addr)];
    std::string   ipAddr;
    std::string   yamlDoc;
    std::string   outputDir;

    while((c =  getopt(argc, argv, "a:Y:s:d:h")) != -1)
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
                if (1 != sscanf(optarg, "%zu", &seconds))
                {
                    printf("Invalid number of seconds\n");
                    exit(1);
                }
                break;
            case 'd':
                outputDir = std::string(optarg);
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
        std::cout << "Number of iterations must be greater that 0" << std::endl;
        exit(1);
    }

    if (outputDir.empty())
    {
        std::cout << "Must specify an output diretory." << std::endl;
        exit(1);
    }

    uint8_t     str[256];
    uint64_t    u64;
    uint8_t     hash[20];

    // Capture CRTL+C to allow the program to finish normally, and write
    // the histogram data to file.
    signal(SIGINT, intHandler);

    // Load YAML definition
    std::cout << "Loading YAML description..." << std::endl;
    IYamlSetIP setIP(ipAddr);
    Path root = IPath::loadYamlFile(yamlDoc.c_str(), "NetIODev", NULL, &setIP);

    // Create CPSW interfaces for AxiVersion registers
    ScalVal_RO fpgaVers    = IScalVal_RO::create (root->findByName("/mmio/AmcCarrierCore/AxiVersion/FpgaVersion"));
    ScalVal_RO bldStamp    = IScalVal_RO::create (root->findByName("/mmio/AmcCarrierCore/AxiVersion/BuildStamp"));
    ScalVal_RO gitHash     = IScalVal_RO::create (root->findByName("/mmio/AmcCarrierCore/AxiVersion/GitHash"));

    // Print AxiVersion Information
    std::cout << std::endl;
    std::cout << "AxiVersion:\n" << std::endl;

    fpgaVers->getVal(&u64);
    std::cout << "FPGAVersion: " << u64 << std::endl;

    bldStamp->getVal(str, sizeof(str)/sizeof(str[0]));
    std::cout << "Buil String:" << str << std::endl;

    std::ostringstream aux;
    gitHash->getVal(hash, 20);
    std::cout << "Git hash: ";
    for (int i = 19; i >= 0; --i)
        aux << std::hex << std::setfill('0') << std::setw(2) << unsigned(hash[i]);
    std::string gitHashStr(aux.str());
    std::cout << gitHashStr << std::endl;

    // Start test
    Tester t(root, gitHashStr, outputDir);

    // Now wait for the defined time
    std::cout << "Now waiting for " << seconds << " seconds..."<< std::endl;
    std::cout << std::endl;

    std::this_thread::sleep_for( std::chrono::seconds( seconds ) );

    std::cout << "End of program." << std::endl;

    return 0;
}
