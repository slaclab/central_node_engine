#include <iostream>
#include <fstream>
#include <stdio.h>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_engine.h>
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

class TestFailed {};

static void usage(const char *nm) {
  std::cerr << "Usage: " << nm << " -f <file> -i <file>" << std::endl;
  std::cerr << "       -f <file>   :  MPS database YAML file" << std::endl;
  std::cerr << "       -i <file>   :  digital input test file" << std::endl;
  std::cerr << "       -a <file>   :  analog input test file" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}


class BypassTest {
 public:
  BypassTest(EnginePtr e) : engine(e), digital(false), analog(false) {};

  EnginePtr engine;
  int testInputCount;
  int analogInputCount;
  std::ifstream testInputFile;
  std::ifstream analogInputFile;
  bool digital;
  bool analog;

  void checkBypass(time_t t) {
    engine->bypassManager->checkBypassQueue(t);
  }

  void addBypass() {
    // Now bypass the OTR IN limit switch to 0 and
    // OTR OUT limit switch to 1, i.e. screen is OUT
    engine->bypassManager->setBypass(engine->mpsDb, BYPASS_DIGITAL, 1, /* deviceId */
				     1 /* bypass value */, 100 /* until */, true);
    engine->bypassManager->setBypass(engine->mpsDb, BYPASS_DIGITAL, 2, /* deviceId */
				     0 /* bypass value */, 100 /* until*/, true);
    
    // OTR Attr both out
    engine->bypassManager->setBypass(engine->mpsDb, BYPASS_DIGITAL, 3, /* deviceId */
				     0 /* bypass value */, 110 /* until */, true);
    engine->bypassManager->setBypass(engine->mpsDb, BYPASS_DIGITAL, 4, /* deviceId */
				     1 /* bypass value */, 110 /* until*/, true);

    // PIC
    engine->bypassManager->setBypass(engine->mpsDb, BYPASS_ANALOG, 1, 0, 115, true);
  }

  void showFaults() {
    engine->mpsDb->showFaults();
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
    float analogValue;

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

      std::cout << "=== Testing digital cycle " << testCycle << " ===" << std::endl;
      for (int i = 0; i < testInputCount; ++i) {
	testInputFile >> deviceId;
	testInputFile >> deviceValue;
	
	//      std::cout << deviceId << ": " << deviceValue << std::endl;
	
	int size = engine->mpsDb->deviceInputs->size() + 1;
	if (deviceId > size) {
	  std::cerr << "ERROR: Can't update device (Id=" << deviceId
		    << "), number of inputs is " << engine->mpsDb->deviceInputs->size()
		    << std::endl;
	  return 1;
	}
	engine->mpsDb->deviceInputs->at(deviceId)->update(deviceValue);
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

      std::cout << "=== Testing analog cycle " << analogCycle << " ===" << std::endl;
      for (int i = 0; i < analogInputCount; ++i) {
	analogInputFile >> deviceId;
	analogInputFile >> analogValue;
	
	//      std::cout << deviceId << ": " << deviceValue << std::endl;
	
	int size = engine->mpsDb->deviceInputs->size() + 1;
	if (deviceId > size) {
	  std::cerr << "ERROR: Can't update device (Id=" << deviceId
		    << "), number of inputs is " << engine->mpsDb->deviceInputs->size()
		    << std::endl;
	  return 1;
	}
	engine->mpsDb->analogDevices->at(deviceId)->update(analogValue);
      }
    }


    //    std::cout << engine->mpsDb->deviceInputs->at(1)->value << std::endl;

    return 0;
  }
};

int main(int argc, char **argv) {
  std::string mpsFileName = "";
  std::string inputFileName = "";
  std::string analogFileName = "";

  for (int opt; (opt = getopt(argc, argv, "hf:i:a:")) > 0;) {
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

  Engine *e = new Engine();
  try {
    if (e->loadConfig(mpsFileName) != 0) {
      std::cerr << "ERROR: Failed to load MPS configuration" << std::endl;
      return 1;
    }
  } catch (DbException ex) {
    std::cerr << ex.what() << std::endl;
    delete e;
    return -1;
  }

  BypassTest *t = new BypassTest(EnginePtr(e));
  
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
  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  //t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  //t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  //t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();

  t->addBypass();
  std::cout << "### BYPASS ADDED" << std::endl;

  // Check bypass expiration - bypass should be all valid
  t->checkBypass(99);

  // Next four updates with bypass VALID
  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  //t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  //t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();

  // Check bypass expiration - bypass should expire now
  t->checkBypass(200);
  std::cout << "### BYPASS EXPIRED" << std::endl;
 
  // Next four updates with bypass EXPIRED
  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  //t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();
  e->showMitigationDevices();
  //t->showFaults();

  t->updateInputsFromTestFile();
  e->checkFaults();
  e->showFaults();


  std::cout << "-------------------------" << std::endl;
  
  e->showStats();

  delete t;

  std::cout << "Done." << std::endl;

  return 0;
}
