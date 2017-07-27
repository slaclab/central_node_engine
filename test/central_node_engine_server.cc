#include <iostream>
#include <fstream>
#include <stdio.h>
#include <signal.h>
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
  std::cerr << "       -v          :  verbose output" << std::endl;
  std::cerr << "       -t          :  trace output" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}

void intHandler(int) {
  exit(0);
}

int main(int argc, char **argv) {
  std::string mpsFileName = "";
  bool verbose = false;
  bool trace = false;

  signal(SIGINT, intHandler);
  
  for (int opt; (opt = getopt(argc, argv, "tvhf:")) > 0;) {
    switch (opt) {
      //    case 'f': doc = YAML::LoadFile(optarg); break;
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

  Engine::getInstance().startUpdateThread();

  while(1) {
    sleep(100);
  };

  std::cout << "Done." << std::endl;

  return 0;
}
