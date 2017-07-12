#ifndef CENTRAL_NODE_FIRMWARE_H
#define CENTRAL_NODE_FIRMWARE_H

#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>

#include <cpsw_api_user.h>

using boost::shared_ptr;

const uint32_t FW_NUM_APPLICATIONS = 8;

/**
 */
class Firmware {
 private:
  Firmware();
  Firmware(Firmware const &);
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
  ScalVal    _configSV[FW_NUM_APPLICATIONS];
  ScalVal    _swEnableSV;
  ScalVal    _swClearSV;
  Stream     _updateStream;

  uint8_t _heartbeat;

 public:
  uint64_t fpgaVersion;
  uint8_t buildStamp[100];
  char gitHashString[21];

  void enable();
  void disable();
  void heartbeat();
  void softwareEnable();
  void softwareDisable();
  void softwareClear();
  
  // size in bytes (not in uint32_t units)
  void writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size);
  uint64_t readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout);

  static Firmware &getInstance() {
    static Firmware instance;
    return instance;
  }

  friend std::ostream & operator<<(std::ostream &os, Firmware * const firmware) {
    os << "=== MpsCentralNode ===" << std::endl;
    os << "FPGA version=" << firmware->fpgaVersion << std::endl;
    os << "Build stamp=\"" << firmware->buildStamp << "\"" << std::endl;
    os << "Git hash=\"" << firmware->gitHashString << "\"" << std::endl;
    /*
    os << "Heartbeat=" << firmware->heartbeat << std::endl;
    os << "Enable=" << firmware->enable << std::endl;
    os << "SW loss error=" << firmware->swLossError << std::endl;
    os << "SW loss count=" << firmware->swLossCnt << std::endl;
    */
    return os;
  }

};

typedef shared_ptr<Firmware> FirmwarePtr;

#endif
