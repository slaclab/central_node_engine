#ifndef CENTRAL_NODE_FIRMWARE_H
#define CENTRAL_NODE_FIRMWARE_H

#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>

#include <cpsw_api_user.h>

#ifndef FW_ENABLED
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using boost::shared_ptr;

const uint32_t FW_NUM_APPLICATIONS = 8;
const uint32_t FW_NUM_BEAM_CLASSES = 16;

// TODO: There is a status register, beamFaultReason. Bits 15:0 indicate a power
// class violation for each destination. Bits 31:16 indicate a min period
// violation for each period. Faults are latched and are cleared with the
// beamFaultClr register.

/**
 */
class Firmware {
 private:
  Firmware();
  Firmware(Firmware const &);
  ~Firmware();
  void operator=(Firmware const &);
  bool initialized;

 public:
  int loadConfig(std::string yamlFileName);

  friend class FirmwareTest;
  
 protected:
  Hub _root;
  Path _path;

  ScalVal_RO _fpgaVersionSV;
  ScalVal_RO _buildStampSV;
  ScalVal_RO _gitHashSV;
  ScalVal    _heartbeatSV;
  ScalVal    _enableSV;
  ScalVal_RO _swLossErrorSV;
  ScalVal_RO _swLossCntSV;
  ScalVal_RO _txClkCntSV; 
  ScalVal    _configSV[FW_NUM_APPLICATIONS];
  ScalVal    _swEnableSV;
  ScalVal    _swClearSV;
  ScalVal    _beamIntTimeSV;
  ScalVal    _beamMinPeriodSV;
  ScalVal    _beamIntChargeSV;
  ScalVal_RO _beamFaultReasonSV;
  ScalVal    _beamFaultEnSV;
  Stream     _updateStreamSV;

  uint8_t _heartbeat;

#ifndef FW_ENABLED
  int _updateSock;
  struct sockaddr_in clientaddr;
#endif

 public:
  uint64_t fpgaVersion;
  uint8_t buildStamp[256];
  char gitHashString[21];

  void enable();
  void disable();
  void heartbeat();
  void softwareEnable();
  void softwareDisable();

  void setEnable(bool enable);
  void setSoftwareEnable(bool enable);
  bool getEnable();
  bool getSoftwareEnable();

  void softwareClear();
  void enableTimingCheck();
  void disableTimingCheck();
  uint32_t getFaultReason();
  void showStats();
  
  uint8_t getSoftwareLossError();
  uint32_t getSoftwareLossCount();
  uint32_t getSoftwareClockCount();

  // size in bytes (not in uint32_t units)
  void writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size);
  uint64_t readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout);
  void writeMitigation(uint32_t *mitigation);
  void writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[]);

  static Firmware &getInstance() {
    static Firmware instance;
    return instance;
  }

  friend std::ostream & operator<<(std::ostream &os, Firmware * const firmware) {
    os << "=== MpsCentralNode ===" << std::endl;
    os << "FPGA version=" << firmware->fpgaVersion << std::endl;
    os << "Build stamp=\"" << firmware->buildStamp << "\"" << std::endl;
    os << "Git hash=\"" << firmware->gitHashString << "\"" << std::endl;

    try {
      uint32_t value;
      firmware->_enableSV->getVal(&value);
      os << "Enable=" << value << std::endl;
      firmware->_swEnableSV->getVal(&value);
      os << "Sw Enable=" << value << std::endl;
    } catch (IOError &e) {
      std::cout << "Exception Info: " << e.getInfo() << std::endl;
      //      firmware->showStats();
    }

    /*
    os << "Heartbeat=" << firmware->heartbeat << std::endl;
    os << "SW loss error=" << firmware->swLossError << std::endl;
    os << "SW loss count=" << firmware->swLossCnt << std::endl;
    */
    return os;
  }

};

typedef shared_ptr<Firmware> FirmwarePtr;

#endif
