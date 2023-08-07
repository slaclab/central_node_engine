#include <iostream>
#include <stdio.h>
#include <fstream>

#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/emit.h>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <boost/shared_ptr.hpp>
#include <log.h>
#include "buffer.h"

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
using namespace easyloggingpp;
#endif

class TestFailed {};

static void usage(const char *nm) {
  std::cerr << "Usage: " << nm << " [-f <file>] [-d]" << std::endl;
  std::cerr << "       -f <file>   :  MPS database YAML file" << std::endl;
  std::cerr << "       -d          :  dump YAML file to dump.txt file" << std::endl;
  std::cerr << "       -t          :  trace output" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}

int main(int argc, char **argv) {
  YAML::Node doc;
  bool loaded = false;
  bool dump = false;
  std::vector<YAML::Node> nodes;
  std::string fileName;
  bool trace = false;

  for (int opt; (opt = getopt(argc, argv, "hdtf:")) > 0;) {
    switch (opt) {
      //    case 'f': doc = YAML::LoadFile(optarg); break;
    case 'f' :
      nodes = YAML::LoadAllFromFile(optarg);
      fileName = optarg;
      loaded = true;
      break;
    case 'd' :
      dump = true;
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

  boost::shared_ptr<MpsDb> mpsDb = boost::shared_ptr<MpsDb>(new MpsDb());

  if (!trace) {
#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
    Configurations c;
    c.setAll(ConfigurationType::Enabled, "false");
    Loggers::setDefaultConfigurations(c, true);
#endif
  }

  if (loaded) {
    try {
      std::cout << "CALL MpsDb::load\n"; // temp
      mpsDb->load(fileName);
      std::cout << "CALL MpsDb::configure\n"; // temp
      // TEMPORARILY COMMENTED OUT
      mpsDb->configure();
    } catch (DbException &e) {
      std::cerr << e.what() << std::endl;
      return -1;
    }
    std::cout << "Database YAML sucessfully loaded and configured, double check the dump.txt if contains all data" << std::endl;
    if (dump) {
      // std::cout << mpsDb << std::endl;
      std::ofstream myfile;
      myfile.open("dump_test.txt");
      myfile << mpsDb;
      myfile.close();
    }
  }
    // Temp - print out the current values grabbed
    // std::cout << "Displaying digital devices\n";
    // printMap<DbDigitalDeviceMapPtr, DbDigitalDeviceMap::iterator>
    // (std::cout, digitalDevices, "DigitalDevice");

  std::cout << "Done." << std::endl;
  /*
  Configurations c;
  //  c.set(Level::All,  ConfigurationType::Enabled, "false");
  c.setAll(ConfigurationType::Format, "[%datetime] %level: %log [%logger]");
  //  c.setAll(ConfigurationType::Filename, "/tmp/custom.log");

  CLOG(INFO, "testLogger") << "Done.";

  // Set default configuration for any future logger - existing logger will not use this configuration unless
  // either true is passed in second argument or set explicitly using Loggers::reconfigureAllLoggers(c);
  Loggers::setDefaultConfigurations(c);
  LOG(INFO) << "Set default configuration but existing loggers not updated yet"; // Logging using trivial logger
  CLOG(INFO, "testLogger") << "Logging using new logger 1"; // You can also use CINFO << "..."
  // Now setting default and also resetting existing loggers
  Loggers::setDefaultConfigurations(c, true);
  c.setAll(ConfigurationType::Enabled, "false");
  c.set(Level::All, ConfigurationType::Enabled, "false");

  Loggers::getLogger("testLogger2");
  Loggers::setDefaultConfigurations(c);

  CLOG(INFO, "testLogger2") << "Logging using new logger 2";
  Loggers::disableAll();
  LOG(INFO) << "Does this show up?";
  CLOG(INFO, "testLogger") << "Does this show up?";
  CLOG(INFO, "testLogger2") << "Does this show up?";
  */
  return 0;
}
