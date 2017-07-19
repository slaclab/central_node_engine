#include <central_node_firmware.h>
#include <central_node_exception.h>
#include <stdint.h>
#include <log_wrapper.h>
#include <stdio.h>

#ifdef LOG_ENABLED
using namespace easyloggingpp;
static Logger *firmwareLogger;
#endif 

#ifdef FW_ENABLED
Firmware::Firmware() :
  initialized(false), _heartbeat(0) {
#ifdef LOG_ENABLED
  firmwareLogger = Loggers::getLogger("FIRMWARE");
  LOG_TRACE("FIRMWARE", "Created Firmware");
#endif
}

int Firmware::loadConfig(std::string yamlFileName) {
  uint8_t gitHash[21];

  _root = IPath::loadYamlFile(yamlFileName.c_str(), "NetIODev")->origin();

  _path = IPath::create(_root);

  std::string base = "/mmio/Kcu105MpsCentralNode";
  std::string axi = "/AxiVersion";
  std::string cn = "/MpsCentralNodeCore";
  std::string config = "/MpsCentralNodeConfig";

  _fpgaVersionSV = IScalVal_RO::create(_path->findByName((base + axi + "/FpgaVersion").c_str()));
  _buildStampSV  = IScalVal_RO::create(_path->findByName((base + axi + "/BuildStamp").c_str()));
  _gitHashSV     = IScalVal_RO::create(_path->findByName((base + axi + "/GitHash").c_str()));
  _heartbeatSV   = IScalVal::create   (_path->findByName((base + cn  + "/SoftwareWdHeartbeat").c_str()));
  _enableSV      = IScalVal::create   (_path->findByName((base + cn  + "/Enable").c_str()));
  _swEnableSV    = IScalVal::create   (_path->findByName((base + cn  + "/SoftwareEnable").c_str()));
  _swClearSV     = IScalVal::create   (_path->findByName((base + cn  + "/SoftwareClear").c_str()));
  _swLossErrorSV = IScalVal_RO::create(_path->findByName((base + cn  + "/SoftwareLossError").c_str()));
  _swLossCntSV   = IScalVal_RO::create(_path->findByName((base + cn  + "/SoftwareLossCnt").c_str()));
  _updateStream  = IStream::create    (_path->findByName("/Stream0"));

  // ScalVal for configuration
  for (int i = 0; i < FW_NUM_APPLICATIONS; ++i) {
    std::stringstream appId;
    appId << "/AppId" << i;
    _configSV[i] = IScalVal::create(_path->findByName((base + config + appId.str()).c_str()));
  }

  // Read some registers that are static
  _fpgaVersionSV->getVal(&fpgaVersion);
  _buildStampSV->getVal(buildStamp, sizeof(buildStamp)/sizeof(buildStamp[0]));
  _gitHashSV->getVal(gitHash, sizeof(gitHash)/sizeof(gitHash[0]));
  for (int i = 0; i < 20; i++) {
    sprintf(&gitHashString[i], "%X", gitHash[i]);
  }
  gitHashString[20] = 0;

  return 0;
}

void Firmware::enable() {
  _enableSV->setVal((uint64_t) 1);
}

void Firmware::disable() {
  _enableSV->setVal((uint64_t) 0);
}

void Firmware::softwareEnable() {
  _swEnableSV->setVal((uint64_t) 1);
}

void Firmware::softwareDisable() {
  _swEnableSV->setVal((uint64_t) 0);
}

void Firmware::softwareClear() {
  _swClearSV->setVal((uint64_t) 1);
  _swClearSV->setVal((uint64_t) 0);
}

void Firmware::heartbeat() {
  _heartbeatSV->setVal((uint64_t)(_heartbeat^=1));
}

void Firmware::writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size) {
  uint32_t *config32 = reinterpret_cast<uint32_t *>(config);
  uint32_t size32 = size / 4;

  if (appNumber < FW_NUM_APPLICATIONS) {
    _configSV[appNumber]->setVal(config32, size);
  }
}

uint64_t Firmware::readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout) {
  return _updateStream->read(buffer, size, CTimeout(timeout));
}

#else
#warning "Code compiled without CPSW - NO FIRMWARE"

//
// If firmware is not used an UDP socket server is created to receive
// simulated link node updates
//
Firmware::Firmware() {
  _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (_sockfd < 0) {
    throw(CentralNodeException("ERROR: Failed to open socket for simulated firmware inputs"));
  }
  
  int optval = 1;
  setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR,
             (const void *) &optval , sizeof(int));

  struct sockaddr_in serveraddr;
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)4356);

  if (bind(_sockfd, (struct sockaddr *) &serveraddr,
           sizeof(serveraddr)) < 0) {
    throw(CentralNodeException("ERROR: Failed to open socket for simulated firmaware inputs"));
  }
};

int Firmware::loadConfig(std::string yamlFileName) {return 0;};
void Firmware::enable() {};
void Firmware::disable() {};
void Firmware::writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size) {};

uint64_t Firmware::readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout) {
  struct sockaddr_in clientaddr;
  socklen_t clientlen = sizeof(clientaddr);

  std::cout << "INFO: waiting for simulated data..." << std::endl;
  int n = recvfrom(_sockfd, buffer, size, 0,
		   (struct sockaddr *) &clientaddr, &clientlen);
  if (n < 0) {
    std::cerr << "ERROR: Failed to receive simulated firmware data" << std::endl;
    return 0;
  }
  else {
    std::cout << "INFO: Received " << n << " bytes." <<std::endl;
  }

 return n;
};
#endif
