#include <central_node_firmware.h>
#include <stdint.h>
#include <log_wrapper.h>


#ifdef LOG_ENABLED
using namespace easyloggingpp;
static Logger *firmwareLogger;
#endif 

#ifdef FW_ENABLED
Firmware::Firmware() :
  initialized(false) {
#ifdef LOG_ENABLED
  firmwareLogger = Loggers::getLogger("FIRMWARE");
  LOG_TRACE("FIRMWARE", "Created Firmware");
#endif
}

int Firmware::loadConfig(std::string yamlFileName) {
  root = IPath::loadYamlFile(yamlFileName.c_str(), "NetIODev")->origin();

  pre = IPath::create(root);

  std::string base = "/mmio/Kcu105MpsCentralNode";
  std::string axi = "/AxiVersion";
  std::string cn = "/MpsCentralNodeCore";

  fpgaVers    = IScalVal_RO::create (pre->findByName((base + axi + "/FpgaVersion").c_str()));
  bldStamp    = IScalVal_RO::create (pre->findByName((base + axi + "/BuildStamp").c_str()));
  gitHash     = IScalVal_RO::create (pre->findByName((base + axi + "/GitHash").c_str()));
  heartBeat   = IScalVal::create    (pre->findByName((base + cn  + "/SoftwareWdHeartbeat").c_str()));
  enable      = IScalVal::create    (pre->findByName((base + cn  + "/Enable").c_str()));
  swLossError = IScalVal_RO::create (pre->findByName((base + cn  + "/SoftwareLossError").c_str()));
  swLossCnt   = IScalVal_RO::create (pre->findByName((base + cn  + "/SoftwareLossCnt").c_str()));

  return 0;
}
#else
#warning "Code compiled without CPSW - NO FIRMWARE"
Firmware::Firmware(){};
int Firmware::loadConfig(std::string yamlFileName) {return 0;};
#endif
