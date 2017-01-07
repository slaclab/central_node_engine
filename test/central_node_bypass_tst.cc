#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdint.h>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_engine.h>
#include <boost/shared_ptr.hpp>
#include <log.h>

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


class BypassTest {
 public:
  //  BypassTest(EnginePtr e) : engine(e), digital(false), analog(false) {};
  BypassTest() : digital(false), analog(false) {};

  //  EnginePtr engine;
  int testInputCount;
  int analogInputCount;
  std::ifstream testInputFile;
  std::ifstream analogInputFile;
  bool digital;
  bool analog;

  void checkBypass(time_t t) {
    Engine::getInstance().bypassManager->checkBypassQueue(t);
  }

  void addBypass() {
    try {
      // Now bypass the OTR IN limit switch to 0 and
      // OTR OUT limit switch to 1, i.e. screen is OUT
      // YAG01_OUT - NOT_OUT
      Engine::getInstance().bypassManager->setBypass(Engine::getInstance().mpsDb, BYPASS_DIGITAL, 1, /* deviceId */
				       1 /* bypass value */, 100 /* until */, true);
      // YAG01_IN - IN
      Engine::getInstance().bypassManager->setBypass(Engine::getInstance().mpsDb, BYPASS_DIGITAL, 2, /* deviceId */
				       0 /* bypass value */, 100 /* until*/, true);
      
      // GUN_TEMP - Fault
      Engine::getInstance().bypassManager->setBypass(Engine::getInstance().mpsDb, BYPASS_DIGITAL, 3, /* deviceId */
				       0 /* bypass value */, 110 /* until */, true);
      // Waveguide TEMP Ok
      Engine::getInstance().bypassManager->setBypass(Engine::getInstance().mpsDb, BYPASS_DIGITAL, 4, /* deviceId */
				       1 /* bypass value */, 110 /* until*/, true);
      
      // BPM01
      Engine::getInstance().bypassManager->setBypass(Engine::getInstance().mpsDb, BYPASS_ANALOG, 11, 7, 300, true);
    } catch (CentralNodeException e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void cancelBypass() {
    try {
      // BPM
      Engine::getInstance().bypassManager->setBypass(Engine::getInstance().mpsDb, BYPASS_ANALOG, 11, 7, 0, true);
    } catch (CentralNodeException e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void showFaults() {
    Engine::getInstance().mpsDb->showFaults();
  }

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

  int updateInputsFromTestFile(bool verbose = false) {
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
    uint32_t deviceId;
    uint32_t deviceValue;
    uint32_t analogValue;

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

      if (verbose) {
	std::cout << "=== Testing digital cycle " << testCycle << " ===" << std::endl;
      }
      for (int i = 0; i < testInputCount; ++i) {
	testInputFile >> deviceId;
	testInputFile >> deviceValue;
	
	//      std::cout << deviceId << ": " << deviceValue << std::endl;
	/* this check is wrong	
	uint32_t size = Engine::getInstance().mpsDb->deviceInputs->size() + 1;
	if (deviceId > size) {
	  std::cerr << "ERROR: Can't update device (Id=" << deviceId
		    << "), number of inputs is " << Engine::getInstance().mpsDb->deviceInputs->size()
		    << std::endl;
	  return 1;
	}
	*/
	Engine::getInstance().mpsDb->deviceInputs->at(deviceId)->update(deviceValue);
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

      if (verbose) {
	std::cout << "=== Testing analog cycle " << analogCycle << " ===" << std::endl;
      }
      for (int i = 0; i < analogInputCount; ++i) {
	analogInputFile >> deviceId;
	analogInputFile >> analogValue;
	
	//      std::cout << deviceId << ": " << deviceValue << std::endl;
	/* this check may not work	
	uint32_t size = Engine::getInstance().mpsDb->deviceInputs->size() + 1;
	if (deviceId > size) {
	  std::cerr << "ERROR: Can't update device (Id=" << deviceId
		    << "), number of inputs is " << Engine::getInstance().mpsDb->deviceInputs->size()
		    << std::endl;
	  return 1;
	}
	*/
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

  for (int opt; (opt = getopt(argc, argv, "hf:i:a:tv")) > 0;) {
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
    case 'h': usage(argv[0]); return 0;
    case 'v':
      verbose = true;
      break;
    case 't':
      trace = true;
      break;
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

  try {
    if (Engine::getInstance().loadConfig(mpsFileName) != 0) {
      std::cerr << "ERROR: Failed to load MPS configuration" << std::endl;
      return 1;
    }
  } catch (DbException ex) {
    std::cerr << ex.what() << std::endl;
    return -1;
  }

  BypassTest *t = new BypassTest();
  
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

  // First four updates without bypass
  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
  }
  t->checkBypass(1);
  //t->showFaults();

  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
  }
  t->checkBypass(1);
  //t->showFaults();

  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
  }
  t->checkBypass(1);
  //t->showFaults();

  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
  }
  t->checkBypass(1);

  t->addBypass();
  if (verbose) {
    std::cout << "### BYPASS ADDED" << std::endl;
  }

  // Check bypass expiration - bypass should be all valid
  t->checkBypass(90);

  // Next four updates with bypass VALID
  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
    t->showFaults();
  }
  t->checkBypass(91);

  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
  }
  t->checkBypass(92);
  //t->showFaults();

  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
  }
  t->checkBypass(93);
  //t->showFaults();
  
  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
  }

  // Check bypass expiration - bypass should expire now
  Engine::getInstance().getBypassManager()->printBypassQueue();
  t->checkBypass(200);
  if (verbose) {
    std::cout << "### BYPASS EXPIRED" << std::endl;
  }
 
  // Next four updates with bypass EXPIRED
  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
    t->showFaults();
  }
  t->checkBypass(201);

  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
  }
  t->checkBypass(202);

  //t->showFaults();

  if (verbose) {
    std::cout << "### CANCEL BPM01 BYPASS" << std::endl;
  }
  t->cancelBypass();
  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    Engine::getInstance().showMitigationDevices();
  }
  t->checkBypass(203);

  if (verbose) {
    t->showFaults();
  }

  t->updateInputsFromTestFile(verbose);
  Engine::getInstance().checkFaults();
  if (verbose) {
    Engine::getInstance().showFaults();
    std::cout << "-------------------------" << std::endl;
    Engine::getInstance().showStats();
  }

  delete t;

  std::cout << "Done." << std::endl;

  return 0;
}
