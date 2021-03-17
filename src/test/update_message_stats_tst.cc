#define _GLIBCXX_USE_NANOSLEEP    // Workaround to use std::this_thread::sleep_for

#include <iostream>
#include <map>
#include <iterator>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <numeric>
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

// Function and Class definitions
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
    ~IYamlSetIP() {}

    virtual void operator()(YAML::Node &root, YAML::Node &top)
    {
        YAML::Node ipAddrNode = IYamlFixup::findByName(root, "ipAddr");

        if ( ! ipAddrNode )
            throw std::runtime_error("ERROR on IYamlSetIP::operator(): 'ipAddr' node was not found!");

        ipAddrNode = ip_addr_.c_str();
    }

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
        file << std::setw(24) << rhs.first << std::setw(16) << rhs.second << "\n";
    };

    template<typename T>
    void write(const T& rhs)
    {
        file << std::setw(24) << rhs << "\n";
    };

private:
    std::string     fileName;
    std::ofstream   file;
};

class Tester
{
public:
    Tester(Path root, const std::string &fwGitHash, const std::string& outputDir, std::size_t strmTimeout);
    ~Tester();

    void rxHandler();

private:
    ScalVal             enable;
    ScalVal             swEnable;
    ScalVal             swClear;
    ScalVal_RO          swLossError;
    ScalVal_RO          swLossCnt;
    Stream              strm;
    std::string         gitHash;
    std::string         outDir;
    RAIIFile            outFileSizes;
    RAIIFile            outFileTSDelta;
    RAIIFile            outFileTS;
    std::size_t         timeout;
    boost::atomic<bool> run;
    std::thread         rxThread;
};

Tester::Tester(Path root, const std::string &fwGitHash, const std::string& outputDir, std::size_t strmTimeout)
:
enable          ( IScalVal::create( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/Enable" ) ) ),
swEnable        ( IScalVal::create( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareEnable" ) ) ),
swClear         ( IScalVal::create( root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareClear") ) ),
swLossError     ( IScalVal_RO::create (root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareLossError") ) ),
swLossCnt       ( IScalVal_RO::create (root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareLossCnt") ) ),
strm            ( IStream::create( root->findByName("/Stream0") ) ),
gitHash         ( fwGitHash ),
outDir          ( outputDir ),
outFileSizes    ( outDir + "/" + gitHash.substr(0, 7) + "_sizes.data" ),
outFileTSDelta  ( outDir + "/" + gitHash.substr(0, 7) + "_ts_delta.data" ),
outFileTS       ( outDir + "/" + gitHash.substr(0, 7) + "_ts.data" ),
timeout         ( strmTimeout ),
run             ( true ),
rxThread        ( std::thread( &Tester::rxHandler, this ) )
{
    std::cout << "Creating Tester object..." << std::endl;

    if ( pthread_setname_np( rxThread.native_handle(), "rxThread" ) )
      perror( "pthread_setname_np failed for rxThread" );

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

    std::size_t                     rxPackages { 0 };   // Number of packet received
    std::size_t                     rxTimeouts { 0 };   // Number of RX timeouts
    int64_t                         got;                // Number of bytes received
    uint8_t                         buf[100*1024];      // 100KBytes buffer
    std::vector<uint64_t>           timeStamps;         // Messages Timestamps
    std::map<int64_t, std::size_t>  histSize;           // Histogram (message sizes)

    // Clear the software error flags before starting
    swClear->setVal((uint64_t)1);
    swClear->setVal((uint64_t)0);

    // Start the software engine
    std::cout << "Enabling MPS FW Software engine..." << std::endl;
    enable->setVal( static_cast<uint64_t>(1) );
    swEnable->setVal( static_cast<uint64_t>(1) );

    // Wait for the first packet
    strm->read(buf, sizeof(buf), CTimeout(10000));

    while (run)
    {
        got = strm->read(buf, sizeof(buf), CTimeout(timeout));
        if ( ! got )
        {
            ++rxTimeouts;
        }
        else
        {
            // Increase RX packet counter
            ++rxPackages;

            // Update the message size histogram
            ++histSize[got];

      	    // Extract the time stamp from the header, and save it
            const uint64_t* ts = reinterpret_cast<const uint64_t*>(&(*(buf + 8)));
            timeStamps.push_back(*ts);
        }

    }

    std::cout << "Rx Thread interrupted" << std::endl;
    std::cout << std::endl;

    // Create the message time stamp delta histogram
    std::vector<int64_t> timeStampsDelta;
    std::adjacent_difference (timeStamps.begin(), timeStamps.end(), back_inserter(timeStampsDelta));
    timeStampsDelta.erase(timeStampsDelta.begin()); // Remove the first element which is not a difference
    std::map<uint32_t, std::size_t> histTSDelta;
    for (std::vector<int64_t>::const_iterator it = timeStampsDelta.begin(); it != timeStampsDelta.end(); ++it)
        ++histTSDelta[*it];

    std::cout << "Rx Thread report:" << std::endl;
    std::cout << "===========================" << std::endl;
    std::cout << "Number of valid packet received : " << rxPackages << std::endl;
    std::cout << "Number of timeouts              : " << rxTimeouts << std::endl;
    std::cout << "Stream read timeout used (us)   : " << timeout    << std::endl;

    // Do not print this info if not packet was received
    if (rxPackages)
    {
        std::cout << "Min message size (bytes)        : " << histSize.begin()->first    << std::endl;
        std::cout << "Max message size (bytes)        : " << histSize.rbegin()->first   << std::endl;
        std::cout << "Min timestamp delta (ns)        : " << histTSDelta.begin()->first << std::endl;
        std::cout << "Max timestamp delta (ns)        : " << histTSDelta.rbegin()->first << std::endl;
    }

    uint8_t  packetLossError;
    uint32_t packetLossCnt;
    swLossError->getVal(&packetLossError);
    std::cout << "FW SoftwareLossError            : " << unsigned(packetLossError) << std::endl;
    swLossCnt->getVal(&packetLossCnt);
    std::cout << "FW SoftwareLossCnt              : " << unsigned(packetLossCnt) << std::endl;

    // Do not create the data file is not packet was received
    if (rxPackages)
    {
        // Write the message size histogram result to the output file
        std::cout << "Writing data to                 : '" << outFileSizes.getName() << "'... ";

        outFileSizes << "# FW version                      : " << gitHash                  << "\n";
        outFileSizes << "# FW version                      : " << gitHash.c_str()          << "\n";
        outFileSizes << "# Number of valid packet received : " << rxPackages               << "\n";
        outFileSizes << "# Number of timeouts              : " << rxTimeouts               << "\n";
        outFileSizes << "# Min message size (bytes)        : " << histSize.begin()->first  << "\n";
        outFileSizes << "# Max message size (bytes)        : " << histSize.rbegin()->first << "\n";
        outFileSizes << "# FW SoftwareLossCnt              : " << packetLossCnt            << "\n";
        outFileSizes << "#\n";
        outFileSizes << "#" <<std::setw(23) << "MessageSize (bytes)" << std::setw(16) << "Counts" << "\n";
        std::for_each(
            histSize.begin(),
            histSize.end(),
            std::bind(&RAIIFile::writePair<int64_t, std::size_t>, &outFileSizes, std::placeholders::_1));
        std::cout << "done!" << std::endl;

        // Write the timestamp delta histogram result to the output file
        std::cout << "Writing data to                 : '" << outFileTSDelta.getName() << "'... ";

        outFileTSDelta << "# FW version                      : " << gitHash                  << "\n";
        outFileTSDelta << "# FW version                      : " << gitHash.c_str()          << "\n";
        outFileTSDelta << "# Number of valid packet received : " << rxPackages               << "\n";
        outFileTSDelta << "# Number of timeouts              : " << rxTimeouts               << "\n";
        outFileTSDelta << "# Min message size (bytes)        : " << histSize.begin()->first  << "\n";
        outFileTSDelta << "# Max message size (bytes)        : " << histSize.rbegin()->first << "\n";
        outFileTSDelta << "# FW SoftwareLossCnt              : " << packetLossCnt            << "\n";
        outFileTSDelta << "#\n";
        outFileTSDelta << "#" << std::setw(23) << "Timestamp delta (ns)" << std::setw(16) << "Counts" << "\n";
        std::for_each(
            histTSDelta.begin(),
            histTSDelta.end(),
            std::bind(&RAIIFile::writePair<int64_t, std::size_t>, &outFileTSDelta, std::placeholders::_1));
        std::cout << "done!" << std::endl;

        // Write the full list of timestamps to the output file
        std::cout << "Writing data to                 : '" << outFileTS.getName() << "'... ";

        outFileTS << "# FW version                      : " << gitHash                  << "\n";
        outFileTS << "# FW version                      : " << gitHash.c_str()          << "\n";
        outFileTS << "# Number of valid packet received : " << rxPackages               << "\n";
        outFileTS << "# Number of timeouts              : " << rxTimeouts               << "\n";
        outFileTS << "# Min message size (bytes)        : " << histSize.begin()->first  << "\n";
        outFileTS << "# Max message size (bytes)        : " << histSize.rbegin()->first << "\n";
        outFileTS << "# FW SoftwareLossCnt              : " << packetLossCnt            << "\n";
        outFileTS << "#\n";
        outFileTS << "#" << std::setw(23) << "Timestamp (ns)" << "\n";
        std::for_each(
            timeStamps.begin(),
            timeStamps.end(),
            std::bind(&RAIIFile::write<uint64_t>, &outFileTS, std::placeholders::_1));
        std::cout << "done!" << std::endl;
    }
    else
    {
        std::cout << "Data files not created, as no valid packet was received." << std::endl;
    }

    std::cout << std::endl;
}

void usage(char* name)
{
    std::cout << "This program collects the messages sizes and timestamp from the update messages send from FW to SW." << std::endl;
    std::cout << "The program runs for the specified number of seconds printing a report at the end." << std::endl;
    std::cout << "The program generates 2 histograms for the message size and the timestamp difference between adjacent messages." << std::endl;
    std::cout << " The histogram files are written to the specified directory. The file name will be firmware short githash (7 chars) "
                 "plus the suffix \"_sizes\" for the message size histogram file, and \"_ts_deta\" for the timestamp delta histogram "
                 "the plus a \".data\" extension." << std::endl;
    std::cout << "Additionally, the program generate a third file with the full list of the received timestamps. The file name will be "
                 "firmware short githash (7 chars), plus the suffix \"_ts\", plus a \".data\" extension." << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << name << " -a <IP_address> -Y <Yaml_top> -s <seconds to run the test> -d <output directory> [-t <timeout>] [-h]" << std::endl;
    std::cout << std::endl;
}

// Main body
int main(int argc, char **argv)
{
    int           c;
    unsigned char buf[sizeof(struct in6_addr)];
    std::string   ipAddr;
    std::string   yamlDoc;
    std::string   outputDir;
    std::size_t   seconds {    0 };
    std::size_t   timeout { 3500 };

    while((c =  getopt(argc, argv, "a:Y:s:d:t:h")) != -1)
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
            case 't':
                if (1 != sscanf(optarg, "%zu", &timeout))
                {
                    printf("Invalid timeout\n");
                    exit(1);
                }
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
        std::cout << "Must specify a YAML top path." << std::endl;
        exit(1);
    }

    if (seconds <= 0)
    {
        std::cout << "Number of iterations must be greater that 0" << std::endl;
        exit(1);
    }

    if (outputDir.empty())
    {
        std::cout << "Must specify an output directory." << std::endl;
        exit(1);
    }

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
    Tester t(root, gitHashStr, outputDir, timeout);

    // Now wait for the defined time
    std::cout << "Now waiting for " << seconds << " seconds..."<< std::endl;
    std::cout << std::endl;

    std::this_thread::sleep_for( std::chrono::seconds( seconds ) );

    std::cout << "End of program." << std::endl;

    return 0;
}

