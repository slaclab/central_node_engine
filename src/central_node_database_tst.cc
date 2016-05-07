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

  for (int opt; (opt = getopt(argc, argv, "hdf:")) > 0;) {
    switch (opt) {
      //    case 'f': doc = YAML::LoadFile(optarg); break;
    case 'f' :
      nodes = YAML::LoadAllFromFile(optarg);
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

  if (loaded && dump) {
    for (std::vector<YAML::Node>::iterator node = nodes.begin();
	 node != nodes.end(); ++node) {
      std::string s = YAML::Dump(*node);
      size_t found = s.find(":");
      if (found == std::string::npos) {
	std::cerr << "ERROR: Can't find Node name in YAML file" << std::endl;
      }
      std::string nodeName = s.substr(0, found);

      //      std::cout << "Processing " << nodeName << std::endl;
      if (nodeName == "Crate") {
	mpsDb->crates = (*node).as<DbCrateListPtr>();
      } else if (nodeName == "ApplicationType") {
	mpsDb->applicationTypes = (*node).as<DbApplicationTypeListPtr>();
      } else if (nodeName == "ApplicationCard") {
	mpsDb->applicationCards = (*node).as<DbApplicationCardListPtr>();
      } else if (nodeName == "DigitalChannel") {
	mpsDb->digitalChannels = (*node).as<DbChannelListPtr>();
      } else if (nodeName == "AnalogChannel") {
	mpsDb->analogChannels = (*node).as<DbChannelListPtr>();
      } else if (nodeName == "DeviceType") {
	mpsDb->deviceTypes = (*node).as<DbDeviceTypeListPtr>();
      } else if (nodeName == "DeviceState") {
	mpsDb->deviceStates = (*node).as<DbDeviceStateListPtr>();
      } else if (nodeName == "DigitalDevice") {
	mpsDb->digitalDevices = (*node).as<DbDigitalDeviceListPtr>();
      } else if (nodeName == "DeviceInput") {
	mpsDb->deviceInputs = (*node).as<DbDeviceInputListPtr>();
      } else if (nodeName == "Fault") {
	mpsDb->faults = (*node).as<DbFaultListPtr>();
      } else if (nodeName == "FaultInput") {
	mpsDb->faultInputs = (*node).as<DbFaultInputListPtr>();
      } else if (nodeName == "DigitalFaultState") {
	mpsDb->digitalFaultStates = (*node).as<DbDigitalFaultStateListPtr>();
      } else if (nodeName == "ThresholdValueMap") {
	mpsDb->thresholdValueMaps = (*node).as<DbThresholdValueMapListPtr>();
      } else if (nodeName == "ThresholdValue") {
	mpsDb->thresholdValues = (*node).as<DbThresholdValueListPtr>();
      } else if (nodeName == "ThresholdFault") {
	mpsDb->thresholdFaults = (*node).as<DbThresholdFaultListPtr>();
      } else if (nodeName == "ThresholdFaultState") {
	mpsDb->thresholdFaultStates = (*node).as<DbThresholdFaultStateListPtr>();
      } else if (nodeName == "MitigationDevice") {
	mpsDb->mitigationDevices = (*node).as<DbMitigationDeviceListPtr>();
      } else if (nodeName == "BeamClass") {
	mpsDb->beamClasses = (*node).as<DbBeamClassListPtr>();
      } else if (nodeName == "AllowedClass") {
	mpsDb->allowedClasses = (*node).as<DbAllowedClassListPtr>();
      }
    }

    std::cout << mpsDb << std::endl;
  }

  std::cout << "Done." << std::endl;

  return 0;
}
