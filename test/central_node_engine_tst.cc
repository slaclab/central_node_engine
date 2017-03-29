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
  std::cerr << "Usage: " << nm << " -f <file> -i <file>" << std::endl;
  std::cerr << "       -f <file>   :  MPS database YAML file" << std::endl;
  std::cerr << "       -i <file>   :  digital input test file" << std::endl;
  std::cerr << "       -a <file>   :  analog input test file" << std::endl;
  std::cerr << "       -v          :  verbose output" << std::endl;
  std::cerr << "       -t          :  trace output" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}


class EngineTest {
 public:
  //  EngineTest(EnginePtr e) : engine(e), digital(false), analog(false) {};
  EngineTest() : digital(false), analog(false) {};

  EnginePtr engine;
  int testInputCount;
  int analogInputCount;
  std::ifstream testInputFile;
  std::ifstream analogInputFile;
  bool digital;
  bool analog;

  // type==0 -> digital
  // type!=0 -> analog
  int loadInputTestFile(std::string testFileName, int type = 0) {
    if (type == 0) {
      testInputFile.open(testFileName.c_str(), std::ifstream::in);
      if (!testInputFile.is_open()) {
	std::cerr << "ERROR: Failed to open test file \"" << testFileName
		  << "\" for reading" << std::endl;
	return 1;
      }
      digital = true;
    }
    else {
      analogInputFile.open(testFileName.c_str(), std::ifstream::in);
      if (!analogInputFile.is_open()) {
	std::cerr << "ERROR: Failed to open analog file \"" << testFileName
		  << "\" for reading" << std::endl;
	return 1;
      }
      analog = true;
    }

    return 0;
  }

  int updateInputsFromTestFile() {
    if (!testInputFile.is_open() && digital) {
      std::cerr << "ERROR: Input test file not opened, can't update inputs."
		<< std::endl;
      return 1;
    }

    if (!analogInputFile.is_open() && analog) {
      std::cerr << "ERROR: Analog test file not opened, can't update inputs."
		<< std::endl;
      return 1;
    }

    std::string s;
    int testCycle;
    int analogCycle;

    // If eof reached, start from beginning
    int deviceId;
    int deviceValue;
    int analogValue;

    if (digital) {
      testInputFile >> s;
      testInputFile >> testInputCount;
      testInputFile >> testCycle;

      if (testInputFile.eof()) {
	testInputFile.clear();
	testInputFile.seekg(0, std::ios::beg);
	
	testInputFile >> s;
	testInputFile >> testInputCount;
	testInputFile >> testCycle;
      }
      
      //      std::cout << "=== Testing digital cycle " << testCycle << " ===" << std::endl;
      for (int i = 0; i < testInputCount; ++i) {
	testInputFile >> deviceId;
	testInputFile >> deviceValue;
	
	//      std::cout << deviceId << ": " << deviceValue << std::endl;
	
	int size = Engine::getInstance().mpsDb->deviceInputs->size() + 1;
	if (deviceId > size) {
	  std::cerr << "ERROR: Can't update device (Id=" << deviceId
		    << "), number of inputs is " << Engine::getInstance().mpsDb->deviceInputs->size()
		    << std::endl;
	  return 1;
	}
	try {
	  Engine::getInstance().mpsDb->deviceInputs->at(deviceId)->update(deviceValue);
	} catch (std::exception e) {
	  std::cerr << "ERROR: invalid device input index of " << deviceId << std::endl;
	  return -1;
	}
      }
    }

    if (analog) {
      analogInputFile >> s;
      analogInputFile >> analogInputCount;
      analogInputFile >> analogCycle;

      if (analogInputFile.eof()) {
	analogInputFile.clear();
	analogInputFile.seekg(0, std::ios::beg);
	
	analogInputFile >> s;
	analogInputFile >> analogInputCount;
	analogInputFile >> analogCycle;
      }

      //      std::cout << "=== Testing analog cycle " << analogCycle << " ===" << std::endl;
      for (int i = 0; i < analogInputCount; ++i) {
	analogInputFile >> deviceId;
	analogInputFile >> analogValue;
	/*	
	std::cout << deviceId << ": " << analogValue << " analogSize: "
	  	  << Engine::getInstance().mpsDb->analogDevices->size() 
	  	  << " digitalSize: "
         	  << Engine::getInstance().mpsDb->digitalDevices->size() 
	    	  << std::endl;
	*/
	int size = Engine::getInstance().mpsDb->analogDevices->size() + 1 + Engine::getInstance().mpsDb->digitalDevices->size();
	if (deviceId > size) {
	  std::cerr << "ERROR: Can't update device (Id=" << deviceId
		    << "), number of inputs is " << Engine::getInstance().mpsDb->deviceInputs->size()
		    << std::endl;
	  return 1;
	}
	Engine::getInstance().mpsDb->analogDevices->at(deviceId)->update(analogValue);
      }
    }


    //    std::cout << Engine::getInstance().mpsDb->deviceInputs->at(1)->value << std::endl;

    return 0;
  }
};

int main(int argc, char **argv) {
  std::string mpsFileName = "";
  std::string inputFileName = "";
  std::string analogFileName = "";
  bool verbose = false;
  bool trace = false;
  int repeat = 1;
  
  for (int opt; (opt = getopt(argc, argv, "tvhf:i:a:r:")) > 0;) {
    switch (opt) {
      //    case 'f': doc = YAML::LoadFile(optarg); break;
    case 'f' :
      mpsFileName = optarg;
      break;
    case 'i':
      inputFileName = optarg;
      break;
    case 'a':
      analogFileName = optarg;
      break;
    case 'v':
      verbose = true;
      break;
    case 't':
      trace = true;
      break;
    case 'r':
      repeat = atoi(optarg);
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

  if (inputFileName == "" && analogFileName == "") {
    std::cerr << "Missing -i option and/or -a option" << std::endl;
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

  //  Engine *e = new Engine();
  try {
    if (Engine::getInstance().loadConfig(mpsFileName) != 0) {
      std::cerr << "ERROR: Failed to load MPS configuration" << std::endl;
      return 1;
    }
  } catch (DbException ex) {
    std::cerr << ex.what() << std::endl;
    //    delete e;
    return -1;
  }

  EngineTest *t = new EngineTest();//EnginePtr(e));
  
  if (inputFileName != "") {
    if (t->loadInputTestFile(inputFileName, 0) != 0) {
      std::cerr << "ERROR: Failed to open input test file" << std::endl;
      return -1;
    }
  }

  if (analogFileName != "") {
    if (t->loadInputTestFile(analogFileName, 1) != 0) {
      std::cerr << "ERROR: Failed to open analog test file" << std::endl;
      return -1;
    }
  }

  for (int i = 0; i < repeat; ++i) {
    t->updateInputsFromTestFile();
    Engine::getInstance().checkFaults();
    if (verbose) {
      Engine::getInstance().showFaults();
      Engine::getInstance().showMitigationDevices();
    }
  }

  //  if (verbose) {  
    std::cout << "-------------------------" << std::endl;
    Engine::getInstance().showStats();
    //  }

  delete t;

  std::cout << "Done." << std::endl;

  return 0;
}
