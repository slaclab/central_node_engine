#include <iostream>
#include <stdio.h>

#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/emit.h>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

class TestFailed {};

static void usage(const char *nm) {
  std::cerr << "Usage: " << nm << " [-f <file>] [-d]" << std::endl;
  std::cerr << "       -f <file>   :  MPS database YAML file" << std::endl;
  std::cerr << "       -d          :  dump YAML file to stdout" << std::endl;
  std::cerr << "       -h          :  print this message" << std::endl;
}

int main(int argc, char **argv) {
  YAML::Node doc;
  bool loaded = false;
  bool dump = false;
  std::vector<YAML::Node> nodes;
  std::string fileName;

  for (int opt; (opt = getopt(argc, argv, "hdf:")) > 0;) {
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
    case 'h': usage(argv[0]); return 0;
    default:
      std::cerr << "Unknown option '" << opt << "'"  << std::endl;
      usage(argv[0]);
    }
  }

  shared_ptr<MpsDb> mpsDb = shared_ptr<MpsDb>(new MpsDb());

  if (loaded) {
    try {
      mpsDb->load(fileName);
      mpsDb->configure();
    } catch (DbException e) {
      std::cerr << e.what() << std::endl;
      return -1;
    }

    if (dump) {
      std::cout << mpsDb << std::endl;
    }
  }

  std::cout << "Done." << std::endl;

  return 0;
}
