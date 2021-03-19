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
    RAIIFile(const std::string &name)
    :
        fileName(name),
        file(name, std::ofstream::out | std::ofstream::binary),
        w1(w1_default),
        w2(w2_default)
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
        file << std::setw(w1) << rhs.first << std::setw(w2) << rhs.second << "\n";
    };

    template<typename T>
    void write(const T& rhs)
    {
        file << std::setw(w1) << rhs << "\n";
    };

    void setWidth(std::size_t w)
    {
        w1 = w;
    }

    void setWidth(const std::pair<std::size_t, std::size_t>& w)
    {
        w1 = w.first;
        w2 = w.second;
    }

    void resetWidth()
    {
        w1 = w1_default;
        w2 = w2_default;
    }

private:
    std::string     fileName;
    std::ofstream   file;
    std::size_t     w1;
    std::size_t     w2;

    static const std::size_t w1_default = 24;
    static const std::size_t w2_default = 16;
};

class Tester
{
public:
    Tester(Path root, const std::string &fwGitHash, const std::string& outputDir, std::size_t strmTimeout);
    ~Tester();

    void rxHandlerMain();
    void rxHandlerSeconday();

private:
    ScalVal             enable;
    ScalVal             swEnable;
    Command             swErrorClear;
    ScalVal_RO          swBusy;
    ScalVal_RO          swPause;
    ScalVal_RO          swLossError;
    ScalVal_RO          swLossCnt;
    ScalVal_RO          swOvflCnt;
    Stream              strm0;
    Stream              strm1;
    std::string         gitHash;
    std::string         outDir;
    RAIIFile            outFileSizes;
    RAIIFile            outFileTSDelta;
    RAIIFile            outFileInfo;
    std::size_t         timeout;
    boost::atomic<bool> run;
    std::thread         rxThread0;
    std::thread         rxThread1;
};

Tester::Tester(Path root, const std::string &fwGitHash, const std::string& outputDir, std::size_t strmTimeout)
:
enable          ( IScalVal::create(    root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/Enable"           ) ) ),
swEnable        ( IScalVal::create(    root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareEnable"   ) ) ),
swErrorClear    ( ICommand::create(    root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SwErrClear"        ) ) ),
swBusy          ( IScalVal_RO::create( root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareBusy"      ) ) ),
swPause         ( IScalVal_RO::create( root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwarePause"     ) ) ),
swLossError     ( IScalVal_RO::create( root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareLossError" ) ) ),
swLossCnt       ( IScalVal_RO::create( root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareLossCnt"   ) ) ),
swOvflCnt       ( IScalVal_RO::create( root->findByName("/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareOvflCnt"   ) ) ),
strm0           ( IStream::create(     root->findByName("/Stream0"                                                         ) ) ),
gitHash         ( fwGitHash ),
outDir          ( outputDir ),
outFileSizes    ( outDir + "/" + gitHash.substr(0, 7) + "_sizes.data"    ),
outFileTSDelta  ( outDir + "/" + gitHash.substr(0, 7) + "_ts_delta.data" ),
outFileInfo     ( outDir + "/" + gitHash.substr(0, 7) + "_info.data"       ),
timeout         ( strmTimeout ),
run             ( true ),
rxThread0       ( std::thread( &Tester::rxHandlerMain,     this ) ),
rxThread1       ( std::thread( &Tester::rxHandlerSeconday, this ) )
{
    std::cout << "Creating Tester object..." << std::endl;

    if ( pthread_setname_np( rxThread0.native_handle(), "rxThread0" ) )
      perror( "pthread_setname_np failed for rxThread0" );


    if ( pthread_setname_np( rxThread1.native_handle(), "rxThread1" ) )
      perror( "pthread_setname_np failed for rxThread1" );

    std::cout << "Tester created." << std::endl;
    std::cout << std::endl;
}

Tester::~Tester()
{
    std::cout << "Destroying Tester object..." << std::endl;

    std::cout << "Stopping the Rx Thread..." << std::endl;
    run = false;
    rxThread0.join();
    rxThread1.join();

    std::cout << "Disabling MPS FW Software engine..." << std::endl;
    swEnable->setVal( static_cast<uint64_t>(0) );
    enable->setVal( static_cast<uint64_t>(0) );

    std::cout << "Tester destroyed." << std::endl;
    std::cout << std::endl;
}

void Tester::rxHandlerMain()
{
    std::cout << "Rx main thread started" << std::endl;

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

    std::size_t                                 rxPackages { 0 }; // Number of packet received
    std::size_t                                 rxTimeouts { 0 }; // Number of RX timeouts
    int64_t                                     got;              // Number of bytes received
    uint8_t                                     buf[100*1024];    // 100KBytes buffer
    std::vector< std::pair<int64_t, uint64_t> > msgInfo;          // Information about each message (size, timestamp)
    std::map<int64_t, std::size_t>              histSize;         // Histogram (message sizes)

    // Clear the software error flags before starting
    swErrorClear->execute();

    // Start the software engine
    std::cout << "Enabling MPS FW Software engine..." << std::endl;
    enable->setVal( static_cast<uint64_t>(1) );
    swEnable->setVal( static_cast<uint64_t>(1) );

    // Wait for the first packet
    strm0->read(buf, sizeof(buf), CTimeout(10000));

    while (run)
    {
        got = strm0->read(buf, sizeof(buf), CTimeout(timeout));
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
            msgInfo.push_back( std::make_pair(got, *ts) );
        }

    }

    std::cout << "Rx main thread interrupted" << std::endl;
    std::cout << std::endl;

    // Create the message time stamp delta histogram
    std::vector<int64_t> timeStampsDelta;
    for (std::vector< std::pair<int64_t, uint64_t> >::const_iterator it = msgInfo.begin() + 1;
         it != msgInfo.end();
         ++it)
        timeStampsDelta.push_back( it->second - (it - 1)->second );

    std::map<uint32_t, std::size_t> histTSDelta;
    for (std::vector<int64_t>::const_iterator it = timeStampsDelta.begin();
         it != timeStampsDelta.end();
         ++it)
        ++histTSDelta[*it];

    std::cout << "Rx main thread report:" << std::endl;
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

    uint8_t u8;
    swBusy->getVal(&u8);
    std::cout << "FW SoftwareBusy                 : " << unsigned(u8) << std::endl;
    swPause->getVal(&u8);
    std::cout << "FW SoftwarePause                : " << unsigned(u8) << std::endl;
    swLossError->getVal(&u8);
    std::cout << "FW SoftwareLossError            : " << unsigned(u8) << std::endl;

    uint32_t packetLossCnt;
    uint32_t packetOverflowCnt;
    swLossCnt->getVal(&packetLossCnt);
    std::cout << "FW SoftwareLossCnt              : " << unsigned(packetLossCnt) << std::endl;
    swOvflCnt->getVal(&packetOverflowCnt);
    std::cout << "FW SoftwareOvflCnt              : " << unsigned(packetOverflowCnt) << std::endl;

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
        outFileSizes << "# FW SoftwareOvflCnt              : " << packetOverflowCnt        << "\n";
        outFileSizes << "#\n";
        outFileSizes << "#" << std::setw(23) << "MessageSize (bytes)" << std::setw(16) << "Counts" << "\n";
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
        outFileTSDelta << "# FW SoftwareOvflCnt              : " << packetOverflowCnt        << "\n";
        outFileTSDelta << "#\n";
        outFileTSDelta << "#" << std::setw(23) << "Timestamp delta (ns)" << std::setw(16) << "Counts" << "\n";
        std::for_each(
            histTSDelta.begin(),
            histTSDelta.end(),
            std::bind(&RAIIFile::writePair<int64_t, std::size_t>, &outFileTSDelta, std::placeholders::_1));
        std::cout << "done!" << std::endl;

        // Write the full list of timestamps to the output file
        std::cout << "Writing data to                 : '" << outFileInfo.getName() << "'... ";

        outFileInfo << "# FW version                      : " << gitHash                  << "\n";
        outFileInfo << "# FW version                      : " << gitHash.c_str()          << "\n";
        outFileInfo << "# Number of valid packet received : " << rxPackages               << "\n";
        outFileInfo << "# Number of timeouts              : " << rxTimeouts               << "\n";
        outFileInfo << "# Min message size (bytes)        : " << histSize.begin()->first  << "\n";
        outFileInfo << "# Max message size (bytes)        : " << histSize.rbegin()->first << "\n";
        outFileInfo << "# FW SoftwareLossCnt              : " << packetLossCnt            << "\n";
        outFileInfo << "#\n";
        outFileInfo << "#" << std::setw(23) << "Message size (bytes)" << std::setw(24) << "Timestamp (ns)" << "\n";
        outFileInfo.setWidth(std::make_pair(24, 24));
        std::for_each(
            msgInfo.begin(),
            msgInfo.end(),
            std::bind(&RAIIFile::writePair<int64_t, uint64_t>, &outFileInfo, std::placeholders::_1));
        std::cout << "done!" << std::endl;
    }
    else
    {
        std::cout << "Data files not created, as no valid packet was received." << std::endl;
    }

    std::cout << std::endl;
}

void Tester::rxHandlerSeconday()
{
    std::cout << "Rx secondary thread started" << std::endl;

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

    std::size_t rxPackages { 0 }; // Number of packet received
    uint8_t     buf[100*1024];    // 100KBytes buffer

    while(run)
    {
        if(strm1->read(buf, sizeof(buf), CTimeout(timeout)))
            // Increase RX packet counter
            ++rxPackages;
    }

    std::cout << "Rx secondary thread interrupted" << std::endl;
    std::cout << std::endl;

    std::cout << "Rx secondary thread report:" << std::endl;
    std::cout << "===========================" << std::endl;
    std::cout << "Number of packet received : " << rxPackages << std::endl;
    std::cout << std::endl;
}

void usage(const std::string& name)
{
    std::cout << "Usage: " << name << " -a <FPGA_IP> -Y <YAML_FILE>" << std::endl;
    std::cout << "                                -s <TEST_DURATION> -d <OUTPUT_DIR>" << std::endl;
    std::cout << "                                [-t <TIMEOUT>] [-h]" << std::endl;
    std::cout << std::endl;
    std::cout << "    -a <FPGA_IP>       : FPGA IP Address." <<std::endl;
    std::cout << "    -Y <YAML_FILE>     : Path to the top level YAML file." <<std::endl;
    std::cout << "    -s <TEST_DURATION> : Number of second to run the test." <<std::endl;
    std::cout << "    -d <OUTPUT_DIR>    : Path to the output directory for the report files." <<std::endl;
    std::cout << "    -t <TIMEOUT>       : Timeout, in us, while waiting for update messages." << std::endl;
    std::cout << "                         Optional. Defaults to 3500." <<std::endl;
    std::cout << "    -h                 : Show this message." <<std::endl;
    std::cout << std::endl;
    std::cout << "This program collects the size and timestamp from the update messages send" << std::endl;
    std::cout << "from the central node firmware. The program runs for the specified number of" << std::endl;
    std::cout << "seconds, and it prints a report at the end of the test. Additionally, it" << std::endl;
    std::cout << "generates 3 reports files in the specified output directory:" << std::endl;
    std::cout << "- A histogram of the received messages sizes, in bytes." << std::endl;
    std::cout << "  The file name is \"<fw_short_githash>_sizes.data\"." << std::endl;
    std::cout << "- A histogram of the timestamp difference between adjacent messages." << std::endl;
    std::cout << "  The file name is \"<fw_short_githash>_ts_delta.data\"." << std::endl;
    std::cout << "- The full list of message sizes and timestamps of all received messages." << std::endl;
    std::cout << "  The file name is \"<fw_short_githash>_info.data\"." << std::endl;
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
    std::string   appName { basename(argv[0]) };

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
                usage(appName);
                exit(0);
                break;
            default:
                std::cout << "Invalid option." << std::endl;
                std::cout << std::endl;
                usage(appName);
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

