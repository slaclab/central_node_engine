#include <iostream>
#include <fstream>
#include <stdio.h>
#include <exception>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_engine.h>
#include <central_node_firmware.h>

#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

class TestFailed {};

static void usage(const char *nm) {
  std::cerr << "Usage: " << nm << " -f <file> -i <file>" << std::endl;
  std::cerr << "       -f <file>   :  MPS database YAML file" << std::endl;
  std::cerr << "       -i <file>   :  digital input test file" << std::endl;
  std::cerr << "       -a <file>   :  analog input test file" << std::endl;
  std::cerr << "       -r <n>      :  number evaluation cycles" << std::endl;
  std::cerr << "       -v          :  verbose output" << std::endl;
  std::cerr << "       -t          :  trace output" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}

class FirmwareTest {
public:
  FirmwareTest(bool v) : verbose(v) {
  }

  int config(std::string fwFileName, std::string mpsFileName) {
    Firmware::getInstance().loadConfig(fwFileName);

    if (verbose) {
      std::cout << &Firmware::getInstance();
      std::cout << std::endl;
    }

    try {
      if (Engine::getInstance().loadConfig(mpsFileName) != 0) {
	std::cerr << "ERROR: Failed to load MPS configuration" << std::endl;
	return 1;
      }
    } catch (DbException ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
    }
    return 0;
  }

  int writeFirmwareConfig() {
    Engine::getInstance().mpsDb->writeFirmwareConfiguration();
    return 0;
  }

  int readFirmwareConfig() {
    uint32_t appFwConfig[APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4];
    
    for (int appId = 0; appId < 2; ++appId) {
      Firmware::getInstance()._configSV[0]->getVal(&appFwConfig[appId], APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4);

      uint32_t *appConfig = reinterpret_cast<uint32_t *>(Engine::getInstance().mpsDb->fastConfigurationBuffer +
						       0 * APPLICATION_CONFIG_BUFFER_SIZE_BYTES);
      for (int i = 0; i < APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4; ++i) {
	if (appConfig[i] != appFwConfig[i]) {
	  std::cerr << "ERROR: Mismatch at word " << i << ". Found "
		    << std::hex << appFwConfig[i] << ", expected "
		    << appConfig[i] << std::dec << std::endl;
	  return -1;
	}
      }
    }

    return 0;
  }

  int getUpdates() {
    Engine::getInstance().startUpdateThread();
    //    Firmware::getInstance().softwareClear();
    Firmware::getInstance().softwareEnable();
    sleep(3);
    Firmware::getInstance().softwareDisable();
    std::cout << Engine::getInstance().mpsDb->_updateCounter << std::endl;
  }

protected:
  bool verbose;
};

int main(int argc, char **argv) {
  std::string fwFileName = "";
  std::string mpsFileName = "";
  std::string inputFileName = "";
  std::string analogFileName = "";
  bool verbose = false;
  bool trace = false;
  int repeat = 1;
  
  for (int opt; (opt = getopt(argc, argv, "vhf:w:")) > 0;) {
    switch (opt) {
    case 'f' :
      mpsFileName = optarg;
      break;
    case 'w':
      fwFileName = optarg;
      break;
    case 'v':
      verbose = true;
      break;
    case 'h': usage(argv[0]); return 0;
    default:
      std::cerr << "Unknown option '" << opt << "'"  << std::endl;
      usage(argv[0]);
    }
  }

  if (fwFileName == "") {
    std::cerr << "Missing -w option" << std::endl;
    usage(argv[0]);
    return 1;
  }

  if (mpsFileName == "") {
    std::cerr << "Missing -f option" << std::endl;
    usage(argv[0]);
    return 1;
  }

  FirmwareTest t(verbose);
  if (t.config(fwFileName, mpsFileName) < 0) {
    return -1;
  }
  
  t.writeFirmwareConfig();
  if (t.readFirmwareConfig() < 0) {
    return -1;
  };

  t.getUpdates();

  std::cout << "Done." << std::endl;

  return 0;
}
