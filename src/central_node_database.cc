#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/emit.h>

#include <central_node_yaml.h>
#include <central_node_database.h>

#include <iostream>

int MpsDb::configure() {
  // Assign BeamClass and MitigationDevice to AllowedClass
  for (DbAllowedClassMap::iterator it = allowedClasses->begin();
       it != allowedClasses->end(); ++it) {
    int id = (*it).second->beamClassId;
    DbBeamClassMap::iterator beamIt = beamClasses->find(id);
    if (beamIt == beamClasses->end()) {
      std::cerr << "ERROR: Failed to configure database, invalid ID found for BeamClass ("
		 << id << ")" << std::endl;
      return -1;
    }
    (*it).second->beamClass = (*beamIt).second;

    id = (*it).second->mitigationDeviceId;
    DbMitigationDeviceMap::iterator mitigationIt = mitigationDevices->find(id);
    if (mitigationIt == mitigationDevices->end()) {
      std::cerr << "ERROR: Failed to configure database, invalid ID found for MitigationDevices ("
		 << id << ")" << std::endl;
      return -1;
    }
    (*it).second->mitigationDevice = (*mitigationIt).second;
  }

  // Assign DeviceInputs to it's DigitalDevices
  for (DbDeviceInputMap::iterator it = deviceInputs->begin(); 
       it != deviceInputs->end(); ++it) {
    int id = (*it).second->digitalDeviceId;

    DbDigitalDeviceMap::iterator deviceIt = digitalDevices->find(id);
    if (deviceIt == digitalDevices->end()) {
      std::cerr << "ERROR: Failed to configure database, invalid ID found for DigitalDevice ("
		<< id << ")" << std::endl;
      return -1;
    }

    // Create a map to hold deviceInput for the digitalDevice
    if (!(*deviceIt).second->inputDevices) {
      DbDeviceInputMap *deviceInputs = new DbDeviceInputMap();
      (*deviceIt).second->inputDevices = DbDeviceInputMapPtr(deviceInputs);
    }
    (*deviceIt).second->inputDevices->insert(std::pair<int, DbDeviceInputPtr>((*it).second->id,
									      (*it).second));
  }

  // Assing fault inputs to faults
  for (DbFaultInputMap::iterator it = faultInputs->begin();
       it != faultInputs->end(); ++it) {
    int id = (*it).second->faultId;

    DbFaultMap::iterator faultIt = faults->find(id);
    if (faultIt == faults->end()) {
      std::cerr << "ERROR: Failed to configure database, invalid ID found for Fault ("
		<< id << ")" << std::endl;
      return -1;
    }

    // Create a map to hold faultInputs for the fault
    if (!(*faultIt).second->faultInputs) {
      DbFaultInputMap *faultInputs = new DbFaultInputMap();
      (*faultIt).second->faultInputs = DbFaultInputMapPtr(faultInputs);
    }
    (*faultIt).second->faultInputs->insert(std::pair<int, DbFaultInputPtr>((*it).second->id,
									   (*it).second));
  }

  // Assign all DigitalFaultStates to a Fault
  for (DbDigitalFaultStateMap::iterator it = digitalFaultStates->begin();
       it != digitalFaultStates->end(); ++it) {
    int id = (*it).second->faultId;

    DbFaultMap::iterator faultIt = faults->find(id);
    if (faultIt == faults->end()) {
      std::cerr << "ERROR: Failed to configure database, invalid ID found for Fault ("
		<< id << ")" << std::endl;
      return -1;
    }

    // Create a map to hold faultInputs for the fault
    if (!(*faultIt).second->digitalFaultStates) {
      DbDigitalFaultStateMap *digFaultStates = new DbDigitalFaultStateMap();
      (*faultIt).second->digitalFaultStates = DbDigitalFaultStateMapPtr(digFaultStates);
    }
    (*faultIt).second->digitalFaultStates->insert(std::pair<int, DbDigitalFaultStatePtr>((*it).second->id,
											 (*it).second));
  }

  // Assign AllowedClasses to DigitalFaultState or ThresholdFaultState
  // There are multiple AllowedClasses for each Fault, one per MitigationDevice
  for (DbAllowedClassMap::iterator it = allowedClasses->begin();
       it != allowedClasses->end(); ++it) {
    int id = (*it).second->faultStateId;

    DbDigitalFaultStateMap::iterator digFaultIt = digitalFaultStates->find(id);
    if (digFaultIt != digitalFaultStates->end()) {
      (*digFaultIt).second->allowedClass = (*it).second;
      std::cout << (*digFaultIt).second->allowedClass << std::endl;

      // Create a map to hold deviceInput for the digitalDevice
      if (!(*digFaultIt).second->allowedClasses) {
	DbAllowedClassMap *faultAllowedClasses = new DbAllowedClassMap();
	(*digFaultIt).second->allowedClasses = DbAllowedClassMapPtr(faultAllowedClasses);
      }
      (*digFaultIt).second->allowedClasses->insert(std::pair<int, DbAllowedClassPtr>((*it).second->id,
										     (*it).second));
      
    }
    else {
      DbThresholdFaultStateMap::iterator thresFaultIt = thresholdFaultStates->find(id);
      if (thresFaultIt != thresholdFaultStates->end()) {
	(*thresFaultIt).second->allowedClass = (*it).second;
      }
      else {
	std::cerr << "ERROR: Failed to configure database, invalid ID found of FaultState "
		  << id << " for AllowedClass " <<  (*it).second->id << std::endl;
	return -1;
      }
    }
  }

  return 0;
}

int MpsDb::load(std::string yamlFileName) {
  YAML::Node doc;
  std::vector<YAML::Node> nodes;

  try {
    nodes = YAML::LoadAllFromFile(yamlFileName);
  } catch (...) {
    std::cerr << "ERROR: Failed to load YAML file ("
	      << yamlFileName << ")" << std::endl;
    return 1;
  }

  for (std::vector<YAML::Node>::iterator node = nodes.begin();
       node != nodes.end(); ++node) {
    std::string s = YAML::Dump(*node);
    size_t found = s.find(":");
    if (found == std::string::npos) {
      std::cerr << "ERROR: Can't find Node name in YAML file ("
		<< yamlFileName << ")" << std::endl;
      return 1;
    }
    std::string nodeName = s.substr(0, found);

    if (nodeName == "Crate") {
      crates = (*node).as<DbCrateMapPtr>();
    } else if (nodeName == "ApplicationType") {
      applicationTypes = (*node).as<DbApplicationTypeMapPtr>();
    } else if (nodeName == "ApplicationCard") {
      applicationCards = (*node).as<DbApplicationCardMapPtr>();
    } else if (nodeName == "DigitalChannel") {
      digitalChannels = (*node).as<DbChannelMapPtr>();
    } else if (nodeName == "AnalogChannel") {
      analogChannels = (*node).as<DbChannelMapPtr>();
    } else if (nodeName == "DeviceType") {
      deviceTypes = (*node).as<DbDeviceTypeMapPtr>();
    } else if (nodeName == "DeviceState") {
      deviceStates = (*node).as<DbDeviceStateMapPtr>();
    } else if (nodeName == "DigitalDevice") {
      digitalDevices = (*node).as<DbDigitalDeviceMapPtr>();
    } else if (nodeName == "DeviceInput") {
      deviceInputs = (*node).as<DbDeviceInputMapPtr>();
    } else if (nodeName == "Fault") {
      faults = (*node).as<DbFaultMapPtr>();
    } else if (nodeName == "FaultInput") {
      faultInputs = (*node).as<DbFaultInputMapPtr>();
    } else if (nodeName == "DigitalFaultState") {
      digitalFaultStates = (*node).as<DbDigitalFaultStateMapPtr>();
    } else if (nodeName == "ThresholdValueMap") {
      thresholdValueMaps = (*node).as<DbThresholdValueMapEntryMapPtr>();
    } else if (nodeName == "ThresholdValue") {
      thresholdValues = (*node).as<DbThresholdValueMapPtr>();
    } else if (nodeName == "AnalogDeviceType") {
      analogDeviceTypes = (*node).as<DbAnalogDeviceTypeMapPtr>();
    } else if (nodeName == "AnalogDevice") {
      analogDevices = (*node).as<DbAnalogDeviceMapPtr>();
    } else if (nodeName == "ThresholdFault") {
      thresholdFaults = (*node).as<DbThresholdFaultMapPtr>();
    } else if (nodeName == "ThresholdFaultState") {
      thresholdFaultStates = (*node).as<DbThresholdFaultStateMapPtr>();
    } else if (nodeName == "MitigationDevice") {
      mitigationDevices = (*node).as<DbMitigationDeviceMapPtr>();
    } else if (nodeName == "BeamClass") {
      beamClasses = (*node).as<DbBeamClassMapPtr>();
    } else if (nodeName == "AllowedClass") {
      allowedClasses = (*node).as<DbAllowedClassMapPtr>();
    } else {
      std::cerr << "ERROR: Unknown YAML node name ("
		<< nodeName << ")" << std::endl;
      return 1;
    }
  }
  return 0;
}

