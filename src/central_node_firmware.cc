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

  std::string base = "/mmio";
  std::string axi = "/AmcCarrierCore/AxiVersion";
  std::string mps = "/MpsCentralApplication";
  std::string core = "/MpsCentralNodeCore";
  std::string config = "/MpsCentralNodeConfig";

  std::string name;

  try {
    name = base + axi +"/FpgaVersion";
    _fpgaVersionSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + axi +"/BuildStamp";
    _buildStampSV  = IScalVal_RO::create(_path->findByName(name.c_str()));
    
    name = base + axi + "/GitHash";
    _gitHashSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SoftwareWdHeartbeat";
    _heartbeatSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/Enable";
    _enableSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SoftwareEnable";
    _swEnableSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/SoftwareClear";
    _swClearSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/SoftwareLossError";
    _swLossErrorSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/SoftwareLossCnt";
    _swLossCntSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamIntTime";
    _beamIntTime = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamMinPeriod";
    _beamMinPeriod = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamIntCharge";
    _beamIntCharge = IScalVal::create(_path->findByName(name.c_str()));

    name = "/Stream0";
    _updateStream = IStream::create(_path->findByName(name.c_str()));

    // ScalVal for configuration
    for (int i = 0; i < FW_NUM_APPLICATIONS; ++i) {
      std::stringstream appId;
      appId << "/AppId" << i;
      name = base + mps + config + appId.str();
      _configSV[i] = IScalVal::create(_path->findByName(name.c_str()));
    }
  } catch (NotFoundError &e) {
    throw(CentralNodeException("ERROR: Failed to find " + name));
  }

  if (_beamIntTime->getNelms() != FW_NUM_BEAM_CLASSES) {
    throw(CentralNodeException("ERROR: Invalid number of beam classes (BeamIntTime)"));
  }
  if (_beamMinPeriod->getNelms() != FW_NUM_BEAM_CLASSES) {
    throw(CentralNodeException("ERROR: Invalid number of beam classes (BeamMinPeriod)"));
  }
  if (_beamIntCharge->getNelms() != FW_NUM_BEAM_CLASSES) {
    throw(CentralNodeException("ERROR: Invalid number of beam classes (BeamIntCharge)"));
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
  try {
    _enableSV->setVal((uint64_t) 1);
  } catch (IOError &e) {
    std::cerr << "ERROR: CPSW I/O Error on enable" << std::endl;
  }
}

void Firmware::disable() {
  _enableSV->setVal((uint64_t) 0);
}

void Firmware::softwareEnable() {
  try {
    _swEnableSV->setVal((uint64_t) 1);
  } catch (IOError &e) {
    std::cerr << "ERROR: CPSW I/O Error on softwareEnable" << std::endl;
  }
}

void Firmware::softwareDisable() {
  try {
    _swEnableSV->setVal((uint64_t) 1);
  } catch (IOError &e) {
    std::cerr << "ERROR: CPSW I/O Error on softwareDisable: " << e.getInfo() << std::endl;
  }
}

void Firmware::softwareClear() {
  try {
    _swClearSV->setVal((uint64_t) 1);
    _swClearSV->setVal((uint64_t) 0);
  } catch (IOError &e) {
    std::cerr << "ERROR: CPSW I/O Error on softwareClear" << std::endl;
  }

}

void Firmware::heartbeat() {
  _heartbeatSV->setVal((uint64_t)(_heartbeat^=1));
}

void Firmware::writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size) {
  uint32_t *config32 = reinterpret_cast<uint32_t *>(config);
  uint32_t size32 = size / 4;

  if (appNumber < FW_NUM_APPLICATIONS) {
    try {
      _configSV[appNumber]->setVal(config32, size32);
    } catch (InvalidArgError &e) {
      throw(CentralNodeException("ERROR: Failed writing app configuration (InvalidArgError)."));
    } catch (BadStatusError &e) {
      std::cout << "Info: " << e.getInfo() << std::endl;
      throw(CentralNodeException("ERROR: Failed writing app configuration (BadStatusError)"));
    }
  }
}

void Firmware::writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[]) {
  _beamIntTime->setVal(time, FW_NUM_BEAM_CLASSES);
  _beamMinPeriod->setVal(period, FW_NUM_BEAM_CLASSES);
  _beamIntCharge->setVal(charge, FW_NUM_BEAM_CLASSES);
}

uint64_t Firmware::readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout) {
  return _updateStream->read(buffer, size, CTimeout(timeout));
}

void Firmware::writeMitigation(uint32_t *mitigation) {
}

#else

//====================================================================
// The following code is for testing the Central Node software 
// without the Central Node board/firmware.
//====================================================================
#warning "Code compiled without CPSW - NO FIRMWARE"

#include <cstdlib>

//
// If firmware is not used an UDP socket server is created to receive
// simulated link node updates
//
Firmware::Firmware() {
  char *portString = getenv("CENTRAL_NODE_TEST_PORT");
  unsigned short port = 4356;
  if (portString != NULL) {
    port = atoi(portString);
    std::cout << "INFO: Server waiting on test data using port " << port << "." << std::endl;
  }


  _updateSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (_updateSock < 0) {
    throw(CentralNodeException("ERROR: Failed to open socket for simulated firmware inputs"));
  }
  
  int optval = 1;
  setsockopt(_updateSock, SOL_SOCKET, SO_REUSEADDR,
             (const void *) &optval , sizeof(int));

  struct sockaddr_in serveraddr;
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(port);

  if (bind(_updateSock, (struct sockaddr *) &serveraddr,
           sizeof(serveraddr)) < 0) {
    throw(CentralNodeException("ERROR: Failed to open socket for simulated firmaware inputs"));
  }

  fpgaVersion = 1;
  for (int i = 0; i < 256; ++i) {
    buildStamp[i] = 0;
  }
  sprintf(gitHashString, "NONE");


  std::cout <<  ">>> Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
};

int Firmware::loadConfig(std::string yamlFileName) {return 0;};
void Firmware::enable() {};
void Firmware::disable() {};
void Firmware::writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size) {};
void Firmware::softwareEnable() {};
void Firmware::softwareDisable() {};
void Firmware::softwareClear() {};
void Firmware::writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[]) {}

uint64_t Firmware::readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout) {
  socklen_t clientlen = sizeof(clientaddr);

  std::cout << "INFO: waiting for simulated data..." << std::endl;
  int n = recvfrom(_updateSock, buffer, size, 0,
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

void Firmware::writeMitigation(uint32_t *mitigation) {
  socklen_t clientlen = sizeof(clientaddr);
  /*
  for (int i = 0; i < 2; ++i) {
    uint32_t powerClass = mitigation[i];
    for (int j = 0; j < 32; j = j + 4) {
      std::cout << "Destination[" << i * 8 + j/4<< "]: Class[" << ((powerClass >> j) & 0xF) << "]" << std::endl; 
    }
  }
  */
  int n = sendto(_updateSock, mitigation, 2*sizeof(uint32_t), 0, reinterpret_cast<struct sockaddr *>(&clientaddr), clientlen);
}

#endif
