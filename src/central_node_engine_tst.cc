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
  std::cerr << "       -i <file>   :  input test file" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}


class EngineTest {
 public:
  EngineTest(EnginePtr e) : engine(e) {};

  EnginePtr engine;
  int testInputCount;
  std::ifstream testInputFile;

  int loadInputTestFile(std::string testFileName) {
    testInputFile.open(testFileName.c_str(), std::ifstream::in);
    if (!testInputFile.is_open()) {
      std::cerr << "ERROR: Failed to open test file \"" << testFileName
      		<< "\" for reading" << std::endl;
      return 1;
    }

    //    testInputFile >> testInputCount; // read number of inputs
    //    std::cout << "Inputs: " << testInputCount << std::endl;

    return 0;
  }

  int updateInputsFromTestFile() {
    if (!testInputFile.is_open()) {
      std::cerr << "ERROR: Input test file not opened, can't update inputs."
		<< std::endl;
      return 1;
    }

    std::string s;
    int testCycle;
    testInputFile >> s;
    testInputFile >> testInputCount;
    testInputFile >> testCycle;

    // If eof reached, start from beginning
    if (testInputFile.eof()) {
      testInputFile.clear();
      testInputFile.seekg(0, std::ios::beg);

      testInputFile >> s;
      testInputFile >> testInputCount;
      testInputFile >> testCycle;
    }

    int deviceId;
    int deviceValue;

    std::cout << "=== Testing cycle " << testCycle << " ===" << std::endl;
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

    //    std::cout << engine->mpsDb->deviceInputs->at(1)->value << std::endl;

    return 0;
  }
};

int main(int argc, char **argv) {
  std::string mpsFileName = "";
  std::string inputFileName = "";

  for (int opt; (opt = getopt(argc, argv, "hf:i:")) > 0;) {
    switch (opt) {
      //    case 'f': doc = YAML::LoadFile(optarg); break;
    case 'f' :
      mpsFileName = optarg;
      break;
    case 'i':
      inputFileName = optarg;
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

  if (inputFileName == "") {
    std::cerr << "Missing -i option" << std::endl;
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

  EngineTest *t = new EngineTest(EnginePtr(e));
  
  if (t->loadInputTestFile(inputFileName) != 0) {
    std::cerr << "ERROR: Failed to open input test file" << std::endl;
    return -1;
  }

  t->updateInputsFromTestFile();
  e->checkFaults();
  t->updateInputsFromTestFile();
  e->checkFaults();
  t->updateInputsFromTestFile();
  e->checkFaults();
  t->updateInputsFromTestFile();
  e->checkFaults();
  t->updateInputsFromTestFile();
  e->checkFaults();
  t->updateInputsFromTestFile();
  e->checkFaults();
  t->updateInputsFromTestFile();
  e->checkFaults();

  delete t;

  std::cout << "Done." << std::endl;

  return 0;
}
