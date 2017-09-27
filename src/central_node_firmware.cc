#include <central_node_firmware.h>
#include <central_node_exception.h>
#include <stdint.h>
#include <log_wrapper.h>
#include <stdio.h>

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
using namespace easyloggingpp;
static Logger *firmwareLogger;
#endif 

#ifdef FW_ENABLED
Firmware::Firmware() :
  initialized(false), _heartbeat(0) {
#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
  firmwareLogger = Loggers::getLogger("FIRMWARE");
  LOG_TRACE("FIRMWARE", "Created Firmware");
#endif
}

Firmware::~Firmware() {
  setSoftwareEnable(false);
  setEnable(false);
}

// TODO: separate yaml loading and scalval creation into two funcs - so can call yamlloader
int Firmware::loadConfig(std::string yamlFileName) {
  uint8_t gitHash[21];

  _root = IPath::loadYamlFile(yamlFileName.c_str(), "NetIODev")->origin();

  _path = IPath::create(_root);

  std::string base = "/mmio";
  std::string axi = "/AmcCarrierCore/AxiVersion";
  std::string mps = "/MpsCentralApplication";
  std::string core = "/MpsCentralNodeCore";
  std::string config = "/MpsCentralNodeConfig";
  std::string pgp2bAxi = "/Pgp2bAxi";

  std::string name;

  try {
    name = base + mps + pgp2bAxi + "/RxPhyReady";
    _rxPhyReadySV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + pgp2bAxi + "/TxPhyReady";
    _txPhyReadySV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + pgp2bAxi + "/RxLocalLinkReady";
    _rxLocalLinkReadySV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + pgp2bAxi + "/RxRemLinkReady";
    _rxRemLinkReadySV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + pgp2bAxi + "/RxFrameCount";
    _rxFrameCountSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + pgp2bAxi + "/RxFrameErrorCoumt";
    _rxFrameErrorCountSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + axi + "/FpgaVersion";
    _fpgaVersionSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + axi + "/BuildStamp";
    _buildStampSV  = IScalVal_RO::create(_path->findByName(name.c_str()));
    
    name = base + axi + "/GitHash";
    _gitHashSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SoftwareWdHeartbeat";
    _heartbeatSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/SwHeartbeat";
    _swHeartbeatCmd = ICommand::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/EvalLatchClear";
    _evalLatchClearCmd = ICommand::create(_path->findByName(name.c_str()));

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

    name = base + mps + core + "/SoftwareBwidthCnt";
    _txClkCntSV = IScalVal_RO::create (_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamIntTime";
    _beamIntTimeSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamMinPeriod";
    _beamMinPeriodSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamIntCharge";
    _beamIntChargeSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamFaultReason";
    _beamFaultReasonSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamFaultEn";
    _beamFaultEnSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/EvalLatchClear";
    _evalLatchClearCmd = ICommand::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/MonErrClear";
    _monErrClearCmd = ICommand::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/SwErrClear";
    _swErrClearCmd = ICommand::create(_path->findByName(name.c_str()));

    name = base + mps + core  + "/BeamFaultClr";
    _beamFaultClrSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/EvaluationSwPowerLevel";
    _swMitigationSV = IScalVal::create(_path->findByName(name.c_str()));
    
    name = base + mps + core + "/EvaluationFwPowerLevel";
    _fwMitigationSV = IScalVal_RO::create(_path->findByName(name.c_str()));
    
    name = base + mps + core + "/EvaluationLatchedPowerLevel";
    _latchedMitigationSV = IScalVal_RO::create(_path->findByName(name.c_str()));
    
    name = base + mps + core + "/EvaluationPowerLevel";
    _mitigationSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/EvaluationLatchedPowerLevel";
    _latchedMitigationSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SwitchConfig";
    _switchConfigCmd = ICommand::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/ToErrClear";
    _toErrClearCmd = ICommand::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/MoConcErrClear";
    _moConcErrClearCmd = ICommand::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/MonitorReady";
    _monitorReadySV = IScalVal_RO::create(_path->findByName(name.c_str()));
    
    name = base + mps + core + "/MonitorRxErrCnt";
    _monitorRxErrorCntSV = IScalVal_RO::create(_path->findByName(name.c_str()));
    
    name = base + mps + core + "/MonitorPauseCnt";
    _monitorPauseCntSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/MonitorOvflCnt";
    _monitorOvflCntSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/MonitorDropCnt";
    _monitorDropCntSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/MonitorConcWdErr"; 
    _monitorConcWdErrSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/MonitorConcStallErr"; 
    _monitorConcStallErrSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/MonitorConcExtRxErr"; 
    _monitorConcExtRxErrSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/TimeoutEnable"; 
    _timeoutEnableSV = IScalVal::create(_path->findByName(name.c_str()));
    
    name = base + mps + core + "/TimeoutClear"; 
    _timeoutClearSV = IScalVal::create(_path->findByName(name.c_str()));
    
    name = base + mps + core + "/TimeoutTime"; 
    _timeoutTimeSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/TimeoutMask"; 
    _timeoutMaskSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/TimeoutErrStatus"; 
    _timeoutErrStatusSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/TimeoutErrIndex"; 
    _timeoutErrIndexSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/TimeoutMsgVer"; 
    _timeoutMsgVerSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/EvaluationEnable";
    _evaluationEnableSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/EvaluationTimeStamp";
    _evaluationTimeStampSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SoftwareWdTime"; 
    _swWdTimeSV = IScalVal::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SoftwareBusy"; 
    _swBusySV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SoftwarePause"; 
    _swPauseSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SoftwareWdError"; 
    _swWdErrorSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = base + mps + core + "/SoftwareOvflCnt"; 
    _swOvflCntSV = IScalVal_RO::create(_path->findByName(name.c_str()));

    name = "/Stream0";
    _updateStreamSV = IStream::create(_path->findByName(name.c_str()));

    // ScalVal for configuration
    for (uint32_t i = 0; i < FW_NUM_APPLICATIONS; ++i) {
      std::stringstream appId;
      appId << "/AppId" << i;
      name = base + mps + config + appId.str();
      _configSV[i] = IScalVal::create(_path->findByName(name.c_str()));
    } 
  } catch (NotFoundError &e) {
    throw(CentralNodeException("ERROR: Failed to find " + name));
  } catch (InterfaceNotImplementedError &e) {
    throw(CentralNodeException("ERROR: Wrong interface for " + name));
  }

  if (_beamIntTimeSV->getNelms() != FW_NUM_BEAM_CLASSES) {
    throw(CentralNodeException("ERROR: Invalid number of beam classes (BeamIntTime)"));
  }
  if (_beamMinPeriodSV->getNelms() != FW_NUM_BEAM_CLASSES) {
    throw(CentralNodeException("ERROR: Invalid number of beam classes (BeamMinPeriod)"));
  }
  if (_beamIntChargeSV->getNelms() != FW_NUM_BEAM_CLASSES) {
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

void Firmware::setBoolU64(ScalVal reg, bool enable) {
  try {
    uint64_t value = 0;
    if (enable) value = 1;
    reg->setVal(value);
  } catch (IOError &e) {
    std::cerr << "ERROR: CPSW I/O Error setBoolU64(reg, bool)" << std::endl;
  }
}

bool Firmware::getBoolU64(ScalVal reg) {
  uint64_t value;
  try {
    reg->getVal(&value);
  } catch (IOError &e) {
    std::cerr << "ERROR: CPSW I/O Error on getBoolU64(reg)" << std::endl;
    return false;
  }

  if (value == 0) return false; else return true;
}

uint32_t Firmware::getUInt32(ScalVal_RO reg) {
  uint32_t value = 0;
  try {
    reg->getVal(&value);
  } catch (IOError &e) {
    std::cerr << "ERROR: CPSW I/O Error on getUInt32(reg)" << std::endl;
  }
  return value;
}

uint8_t Firmware::getUInt8(ScalVal_RO reg) {
  uint8_t value = 0;
  try {
    reg->getVal(&value);
  } catch (IOError &e) {
    std::cerr << "ERROR: CPSW I/O Error on getUInt8(reg)" << std::endl;
  }
  return value;
}

void Firmware::setEnable(bool enable) {
  setBoolU64(_enableSV, enable);
}

bool Firmware::getEnable() {
  return getBoolU64(_enableSV);
}

uint32_t Firmware::getSoftwareClockCount() {
  return getUInt32(_txClkCntSV);
}

uint8_t Firmware::getSoftwareLossError() {
  return getUInt8(_swLossErrorSV);
}

uint32_t Firmware::getSoftwareLossCount() {
  return getUInt32(_swLossCntSV);
}

void Firmware::setEvaluationEnable(bool enable) {
  setBoolU64(_evaluationEnableSV, enable);
}

bool Firmware::getEvaluationEnable() {
  return getBoolU64(_evaluationEnableSV);
}

void Firmware::setTimeoutEnable(bool enable) {
  setBoolU64(_timeoutEnableSV, enable);
}

bool Firmware::getTimeoutEnable() {
  return getBoolU64(_timeoutEnableSV);
}

void Firmware::setTimingCheckEnable(bool enable) {
  setBoolU64(_beamFaultEnSV, enable);
}

void Firmware::setSoftwareEnable(bool enable) {
  setBoolU64(_swEnableSV, enable);
}

bool Firmware::getSoftwareEnable() {
  return getBoolU64(_swEnableSV);
}

bool Firmware::getTimingCheckEnable() {
  return getBoolU64(_beamFaultEnSV);
}

void Firmware::softwareClear() {
  setBoolU64(_swClearSV, true);
  setBoolU64(_swClearSV, false);
}

uint32_t Firmware::getFaultReason() {
  return getUInt32(_beamFaultReasonSV);
}

void Firmware::getFirmwareMitigation(uint32_t *fwMitigation) {
  _fwMitigationSV->getVal(fwMitigation, 2);
}

void Firmware::getSoftwareMitigation(uint32_t *swMitigation) {
  _swMitigationSV->getVal(swMitigation, 2);
}

void Firmware::getMitigation(uint32_t *mitigation) {
  _mitigationSV->getVal(mitigation, 2);
}

void Firmware::getLatchedMitigation(uint32_t *latchedMitigation) {
  _latchedMitigationSV->getVal(latchedMitigation, 2);
}

void Firmware::extractMitigation(uint32_t *compressed, uint8_t *expanded) {
  for (int i = 0; i < 8; ++i) {
    expanded[i] = (uint8_t)((compressed[0] >> (4 * i)) & 0xF);
  }

  for (int i = 0; i < 8; ++i) {
    expanded[i+8] = (uint8_t)((compressed[1] >> (4 * i)) & 0xF);
  }
}

void Firmware::heartbeat() {
  _swHeartbeatCmd->execute();
}

void Firmware::writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size) {
  uint32_t *config32 = reinterpret_cast<uint32_t *>(config);
  uint32_t size32 = size / 4;

  if (appNumber < FW_NUM_APPLICATIONS) {
    try {
      LOG_TRACE("FIRMWARE", "Writing configuration for application number #" << appNumber << " data size=" << size32);
      _configSV[appNumber]->setVal(config32, size32);
    } catch (InvalidArgError &e) {
      throw(CentralNodeException("ERROR: Failed writing app configuration (InvalidArgError)."));
    } catch (BadStatusError &e) {
      std::cout << "Exception Info: " << e.getInfo() << std::endl;
      throw(CentralNodeException("ERROR: Failed writing app configuration (BadStatusError)"));
    } catch (IOError &e) {
      std::cout << "Exception Info: " << e.getInfo() << std::endl;
      showStats();
      throw(CentralNodeException("ERROR: Failed writing app configuration (IOError)"));
    }
  }

  // Hit the switch register to enable now configuration
  switchConfig();
}

void Firmware::switchConfig() {
  _switchConfigCmd->execute();
}

void Firmware::evalLatchClear() {
  _evalLatchClearCmd->execute();
}

void Firmware::monErrClear() {
  _monErrClearCmd->execute();
}

void Firmware::swErrClear() {
  _swErrClearCmd->execute();
}

void Firmware::toErrClear() {
  _toErrClearCmd->execute();
}

void Firmware::moConcErrClear() {
  _moConcErrClearCmd->execute();
}

void Firmware::writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[]) {
  _beamIntTimeSV->setVal(time, FW_NUM_BEAM_CLASSES);
  _beamMinPeriodSV->setVal(period, FW_NUM_BEAM_CLASSES);
  _beamIntChargeSV->setVal(charge, FW_NUM_BEAM_CLASSES);
}

uint64_t Firmware::readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout) {
  uint64_t val = 0;
  try {
    val = _updateStreamSV->read(buffer, size, CTimeout(timeout));
  } catch (IOError &e) {
    std::cout << "Failed to read update stream exception: " << e.getInfo() << std::endl;
    val = 0;
  }
  return val;
}

void Firmware::writeMitigation(uint32_t *mitigation) {
  _swMitigationSV->setVal(mitigation, 2);
}

void Firmware::showStats() {
  Children rootsChildren = _path->origin()->getChildren();
  std::vector<Child>::const_iterator it;

  for ( it = rootsChildren->begin(); it != rootsChildren->end(); ++it ) {
    (*it)->dump();
  }
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

int Firmware::loadConfig(std::string yamlFileName) {
  std::cout << ">>> Firmware::loadConfig(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
  return 0;
};

void Firmware::setEnable(bool enable) {
  std::cout << ">>>  Firmware::disable(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
}

void Firmware::setSoftwareEnable(bool enable) {
  std::cout << ">>>  Firmware::disable(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
}

void Firmware::getSoftwareEnable() {
  std::cout << ">>>  Firmware::disable(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
  return false;
}

void Firmware::writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size) {
  std::cout << ">>> Firmware::writeConfig(" << appNumber << "): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
};

void Firmware::softwareEnable()  {
  std::cout << ">>>  Firmware::softwareEnable(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
};

void Firmware::softwareDisable()  {
  std::cout << ">>> Firmware::softwareDisable(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
};

void Firmware::softwareClear() {
  std::cout << ">>>  Firmware::softwareClear(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
};

void Firmware::writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[]) {
  std::cout << ">>> Firmware::writeTimingChecking(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
};

void Firmware::showStats() {
  std::cout << ">>> Firmware::showStats(): Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
}

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
