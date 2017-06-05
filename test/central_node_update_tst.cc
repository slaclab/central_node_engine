#include <iostream>
#include <fstream>
#include <stdio.h>
#include <exception>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_engine.h>

#include <boost/shared_ptr.hpp>
//#include <log.h>
#include <log_wrapper.h>

#ifdef LOG_ENABLED
using namespace easyloggingpp;
#endif 
using boost::shared_ptr;

class TestFailed {};

static void usage(const char *nm) {
  std::cerr << "Usage: " << nm << " -f <file>" << std::endl;
  std::cerr << "       -f <file>   :  MPS database YAML file" << std::endl;
  std::cerr << "       -v          :  verbose output" << std::endl;
  std::cerr << "       -t          :  trace output" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}

class EngineTest {
 public:
  //  EngineTest(EnginePtr e) : engine(e), digital(false), analog(false) {};
  EngineTest() : digital(false), analog(false) {
    initUpdateBuffer();
  };

  void initUpdateBuffer() {
    MpsDbPtr mpsDb = Engine::getInstance().getCurrentDb();
    
    for (DbApplicationCardMap::iterator app = mpsDb->applicationCards->begin();
	 app != mpsDb->applicationCards->end(); ++app) {
      if ((*app).second->isDigital()) {
	initDigitalUpdateBuffer(&mpsDb->getFastUpdateBuffer()[(*app).second->globalId *
							      APPLICATION_UPDATE_BUFFER_SIZE_BYTES]);
      }
      else if ((*app).second->isAnalog()) {
	initAnalogUpdateBuffer(&mpsDb->getFastUpdateBuffer()[(*app).second->globalId *
							     APPLICATION_UPDATE_BUFFER_SIZE_BYTES]);
      }
    }
  }

  void initDigitalUpdateBuffer(char *buffer) {
    char status = 0xAA; // Was High=1, Was Low=0 for 4 channels
    for (int i = 0; i < APPLICATION_UPDATE_BUFFER_SIZE_BYTES; ++i) {
      buffer[i] = status;
    }
  }

  void initAnalogUpdateBuffer(char *buffer) {
    char status = 0xAA; // Was High=1, Was Low=0 for 4 channels
    for (int i = 0; i < APPLICATION_UPDATE_BUFFER_SIZE_BYTES; ++i) {
      buffer[i] = status;
    }
  }
    

  // Memory update buffer that simulates data coming from firmware
  //  char fastUpdateBuffer[NUM_APPLICATIONS * APPLICATION_UPDATE_BUFFER_SIZE_BYTES];

  int testInputCount;
  int analogInputCount;
  std::ifstream testInputFile;
  std::ifstream analogInputFile;
  bool digital;
  bool analog;
};

int main(int argc, char **argv) {
  std::string mpsFileName = "";
  bool verbose = false;
  bool trace = false;
  
  for (int opt; (opt = getopt(argc, argv, "tvhf:")) > 0;) {
    switch (opt) {
    case 'f' :
      mpsFileName = optarg;
      break;
    case 'v':
      verbose = true;
      break;
    case 't':
      trace = true;
      break;
    case 'h': usage(argv[0]); return 0;
    default:
      std::cerr << "Unknown option '" << opt << "'"  << std::endl;
      usage(argv[0]);
    }
  }

  if (mpsFileName == "") {
    std::cerr << "Missing -f option" << std::endl;
    usage(argv[0]);
    return 1;
  }

#ifdef LOG_ENABLED
  if (!trace) {
    Configurations c;
    c.setAll(ConfigurationType::Enabled, "false");
    Loggers::setDefaultConfigurations(c, true);
  }
#endif

  try {
    if (Engine::getInstance().loadConfig(mpsFileName) != 0) {
      std::cerr << "ERROR: Failed to load MPS configuration" << std::endl;
      return 1;
    }
  } catch (DbException ex) {
    std::cerr << ex.what() << std::endl;
    return -1;
  }

  EngineTest *t = new EngineTest();
  Engine::getInstance().getCurrentDb()->updateInputs();
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
  }

  //  if (verbose) {  
    std::cout << "-------------------------" << std::endl;
    Engine::getInstance().showStats();
    //  }

  delete t;

  std::cout << "Done." << std::endl;

  return 0;
}
