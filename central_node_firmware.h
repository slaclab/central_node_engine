#ifndef CENTRAL_NODE_FIRMWARE_H
#define CENTRAL_NODE_FIRMWARE_H

#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>

#include <cpsw_api_user.h>

using boost::shared_ptr;

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

  //  friend class FirmwareTest;
  //  friend class BypassTest;
  
 protected:
  Hub root;
  Path pre;

  ScalVal_RO fpgaVers;
  ScalVal_RO bldStamp;
  ScalVal_RO gitHash;
  ScalVal    heartBeat;
  ScalVal    enable;
  ScalVal_RO swLossError;
  ScalVal_RO swLossCnt;

 public:
  static Firmware &getInstance() {
    static Firmware instance;
    return instance;
  }
};

typedef shared_ptr<Firmware> FirmwarePtr;

#endif
