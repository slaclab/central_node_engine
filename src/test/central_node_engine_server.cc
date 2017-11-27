#include <iostream>
#include <fstream>
#include <stdio.h>
#include <signal.h>
#include <exception>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_engine.h>
#include <central_node_firmware.h>
#include <central_node_history.h>

#include <boost/shared_ptr.hpp>
//#include <log.h>
#include <log_wrapper.h>

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
using namespace easyloggingpp;
#endif 
using boost::shared_ptr;

class TestFailed {};

static void usage(const char *nm) {
  std::cerr << "Usage: " << nm << " -f <file> -i <file>" << std::endl;
  std::cerr << "       -f <file>   :  MPS database YAML file" << std::endl;
#ifdef FW_ENABLED
  std::cerr << "       -w <file>   :  central node firmware YAML config file" << std::endl;
#endif
  std::cerr << "       -v          :  verbose output" << std::endl;
  std::cerr << "       -t          :  trace output" << std::endl;
  std::cerr << "       -c          :  clear firmware - disable MPS" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}

void intHandler(int) {
  Firmware::getInstance().setSoftwareEnable(false);
  Firmware::getInstance().setEnable(false);

  exit(0);
}

int main(int argc, char **argv) {
  std::string mpsFileName = "";
  std::string fwFileName = "";
  bool verbose = false;
  bool trace = false;
  bool clear = false;

  signal(SIGINT, intHandler);
  
  for (int opt; (opt = getopt(argc, argv, "tvhf:w:c")) > 0;) {
    switch (opt) {
      //    case 'f': doc = YAML::LoadFile(optarg); break;
    case 'f' :
      mpsFileName = optarg;
      break;
    case 'w':
      fwFileName = optarg;
      break;
    case 'v':
      verbose = true;
      break;
    case 't':
      trace = true;
      break;
    case 'c':
      clear = true;
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

#ifdef FW_ENABLED
  if (fwFileName == "") {
    std::cerr << "Missing -w option" << std::endl;
    usage(argv[0]);
    return 1;
  }
  Firmware::getInstance().createRoot(fwFileName);
  Firmware::getInstance().createRegisters();

  if (clear) {
    std::cout << &(Firmware::getInstance());
  }

  Firmware::getInstance().softwareClear();
  Firmware::getInstance().setSoftwareEnable(false);
  Firmware::getInstance().setEnable(false);
  
  if (clear) {
    std::cout << &(Firmware::getInstance());

    return 0;
  }
#endif

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
  if (!trace) {
    Configurations c;
    c.setAll(ConfigurationType::Enabled, "false");
    Loggers::setDefaultConfigurations(c, true);
  }
#endif

  History::getInstance().startSenderThread();

  //  sleep(5);

  try {
    if (Engine::getInstance().loadConfig(mpsFileName) != 0) {
      std::cerr << "ERROR: Failed to load MPS configuration" << std::endl;
      return 1;
    }
  } catch (DbException ex) {
    std::cerr << ex.what() << std::endl;
    return -1;
  }
/*

  std::cout << "Streaming stream for 10 seconds ..." << std::endl;
  sleep(10);
  Firmware::getInstance().setSoftwareEnable(false);
  Engine::getInstance().getCurrentDb()->showInfo();
  std::cout << "Stopping stream for 10 seconds ..." << std::endl;
  sleep(10);
  Engine::getInstance().getCurrentDb()->showInfo();
  std::cout << "Resuming stream for 10 seconds ..." << std::endl;
  Firmware::getInstance().setSoftwareEnable(true);
  sleep(10);
  Engine::getInstance().threadExit();
  Firmware::getInstance().setSoftwareEnable(false);
  Engine::getInstance().getCurrentDb()->showInfo();

  Engine::getInstance().threadJoin();
*/

  for (;;) {sleep(1);};
  std::cout << "Done." << std::endl;

  return 0;
}
