#include <iostream>
#include <fstream>
#include <stdio.h>
#include <exception>
#include <stdlib.h>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_engine.h>
#include <central_node_firmware.h>

#include <pthread.h> // MUTEX TEST

class TestFailed {};

static void usage(const char *nm) {
  std::cerr << "Usage: " << nm << " -f <file> -i <file>" << std::endl;
  std::cerr << "       -f <file>   :  MPS database YAML file" << std::endl;
  std::cerr << "       -w <file>   :  central node firmware YAML config file" << std::endl;
  std::cerr << "       -v          :  verbose output" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}

class FirmwareTest {
public:
  uint32_t appConfigWrite[APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4];
  uint32_t appConfigWrite2[APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4];
  uint32_t appConfigRead[APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4];
  uint32_t numApps;

  FirmwareTest(bool v) : numApps(8), verbose(v) {
    srandom(time(0));
    for (uint32_t i = 0; i < APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4; ++i) {
      appConfigWrite[i] = random();
      appConfigWrite2[i] = random();
      appConfigRead[i] = 0;
    }
  }

  int config(std::string fwFileName) {
    try {
      Firmware::getInstance().createRoot(fwFileName);
      Firmware::getInstance().createRegisters();

      if (verbose) {
	//	std::cout << &Firmware::getInstance();
	//	std::cout << std::endl;
      }

    } catch (DbException &ex) {
      std::cerr << ex.what() << std::endl;
      return -1;
    }
    return 0;
  }

  int writeFirmwareConfig(int buffer = 0) {
    try {
      for (uint32_t appId = 0; appId < numApps; ++appId) {
	if (buffer == 0) {
	  Firmware::getInstance()._configSV[appId]->setVal(appConfigWrite, APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4);
	}
	else {
	  Firmware::getInstance()._configSV[appId]->setVal(appConfigWrite2, APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4);
	}
      }
    } catch (BadStatusError &e) {
      std::cerr << "BadStatusError writing config... no good" <<std::endl;
      return -1;
    }
    return 0;
  }

  int readFirmwareConfig(int buffer = 0) {
    for (uint32_t appId = 0; appId < numApps; ++appId) {
      try {
	Firmware::getInstance()._configSV[appId]->getVal(&appConfigRead[0], APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4);
      } catch (BadStatusError &e) {
	std::cerr << "BadStatusError reading config... no good" <<std::endl;
	return -1;
      }

      for (uint32_t i = 0; i < APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES / 4; ++i) {
	if (buffer == 0) {
	  if (appConfigRead[i] != appConfigWrite[i]) {
	    std::cerr << "ERROR: Mismatch at word " << i << ". Found "
		      << std::hex << appConfigRead[i] << ", expected "
		      << appConfigWrite[i] << std::dec << std::endl;
	    return -1;
	  }
	}
	else {
	  if (appConfigRead[i] != appConfigWrite2[i]) {
	    std::cerr << "ERROR: Mismatch at word " << i << ". Found "
		      << std::hex << appConfigRead[i] << ", expected "
		      << appConfigWrite[i] << std::dec << std::endl;
	    return -1;
	  }
	}
      }
    }

    return 0;
  }

protected:
  bool verbose;
};

int main(int argc, char **argv) {
  std::string fwFileName = "";
  std::string inputFileName = "";
  std::string analogFileName = "";
  bool verbose = false;

  for (int opt; (opt = getopt(argc, argv, "vhw:")) > 0;) {
    switch (opt) {
    case 'w':
      fwFileName = optarg;
#ifndef FW_ENABLED
      std::cout << "Library compiled without -DFW_ENABLED option" << std::endl;
#endif
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

  FirmwareTest t(verbose);
  if (t.config(fwFileName) < 0) {
    return -1;
  }

  if (verbose) {
    std::cout << "INFO: Writing buffer 0... ";
  }
  t.writeFirmwareConfig();
  if (verbose) {
    std::cout << " done." << std::endl;
    std::cout << "INFO: Switch config to buffer 1... ";
  }
  Firmware::getInstance().switchConfig();
  if (verbose) {
    std::cout << " done." << std::endl;
    std::cout << "INFO: Reading buffer 1... ";
  }
  if (t.readFirmwareConfig() < 0) {
    if (verbose) {
      std::cout << "INFO: Failed as expected - compared with values written to buffer 0." << std::endl;
    }
  }
  else {
    std::cerr << "ERROR: Buffer switch did not work!" << std::endl;
  }

  // Now try to read the correct buffer
  if (verbose) {
    std::cout << "INFO: Switch config to buffer 0... ";
  }
  Firmware::getInstance().switchConfig();
  if (verbose) {
    std::cout << " done." << std::endl;
    std::cout << "INFO: Reading buffer 0... ";
  }
  if (t.readFirmwareConfig() < 0) {
    std::cerr << "ERROR: Buffer switch did not work!" << std::endl;
  }
  else {
    if (verbose) {
      std::cout << " ... buffer correct ... done." << std::endl;
    }
  }

  // Now try to write buffer 1
  if (verbose) {
    std::cout << "INFO: Switch config to buffer 1... ";
  }
  Firmware::getInstance().switchConfig();
  if (verbose) {
    std::cout << " done." << std::endl;
    std::cout << "INFO: Writing buffer 1... ";
  }
  t.writeFirmwareConfig(1);
  if (verbose) {
    std::cout << " done." << std::endl;
    std::cout << "INFO: Switch config buffer twice -> 0 then -> 1... ";
  }
  Firmware::getInstance().switchConfig();
  Firmware::getInstance().switchConfig();
  if (verbose) {
    std::cout << " done." << std::endl;
    std::cout << "INFO: Reading buffer 1... ";
  }
  if (t.readFirmwareConfig(1) < 0) {
    std::cerr << "ERROR: Buffer switch did not work!" << std::endl;
  }
  else {
    if (verbose) {
      std::cout << " ... buffer correct ... done." << std::endl;
    }
  }

  if (verbose) {
    std::cout << "INFO: Switch config to buffer 0... ";
  }
  Firmware::getInstance().switchConfig();
  if (verbose) {
    std::cout << " done." << std::endl;
    std::cout << "INFO: Reading buffer 0... ";
  }
  if (t.readFirmwareConfig(1) < 0) {
    if (verbose) {
      std::cout << "INFO: Failed as expected - compared with values written to buffer 1." << std::endl;
    }
  }
  else {
    std::cerr << "ERROR: Buffer switch did not work!" << std::endl;
  }


  std::cout << "Done." << std::endl;

  return 0;
}
