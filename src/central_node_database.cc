#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/emit.h>

#include <central_node_yaml.h>
#include <central_node_database.h>

#include <iostream>
#include <sstream>

#include <log_wrapper.h>

#ifdef LOG_ENABLED
using namespace easyloggingpp;
static Logger *databaseLogger;

MpsDb::MpsDb() {
  databaseLogger = Loggers::getLogger("DATABASE");
}
#else
MpsDb::MpsDb() {
}
#endif

void MpsDb::configureAllowedClasses() {
  std::stringstream errorStream;
  // Assign BeamClass and MitigationDevice to AllowedClass
  for (DbAllowedClassMap::iterator it = allowedClasses->begin();
       it != allowedClasses->end(); ++it) {
    int id = (*it).second->beamClassId;
    DbBeamClassMap::iterator beamIt = beamClasses->find(id);
    if (beamIt == beamClasses->end()) {
      errorStream <<  "ERROR: Failed to configure database, invalid ID found for BeamClass ("
		  << id << ") for AllowedClass (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    (*it).second->beamClass = (*beamIt).second;

    id = (*it).second->mitigationDeviceId;
    DbMitigationDeviceMap::iterator mitigationIt = mitigationDevices->find(id);
    if (mitigationIt == mitigationDevices->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for MitigationDevices ("
		  << id << ") for AllowedClass (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    (*it).second->mitigationDevice = (*mitigationIt).second;
  }

  // Assign AllowedClasses to DigitalFaultState or ThresholdFaultState
  // There are multiple AllowedClasses for each Fault, one per MitigationDevice
  for (DbAllowedClassMap::iterator it = allowedClasses->begin();
       it != allowedClasses->end(); ++it) {
    int id = (*it).second->faultStateId;

    DbDigitalFaultStateMap::iterator digFaultIt = digitalFaultStates->find(id);
    if (digFaultIt != digitalFaultStates->end()) {
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
	// Create a map to hold deviceInput for the digitalDevice
	if (!(*thresFaultIt).second->allowedClasses) {
	  DbAllowedClassMap *faultAllowedClasses = new DbAllowedClassMap();
	  (*thresFaultIt).second->allowedClasses = DbAllowedClassMapPtr(faultAllowedClasses);
	}
	(*thresFaultIt).second->allowedClasses->insert(std::pair<int, DbAllowedClassPtr>((*it).second->id,
											 (*it).second));
      }
      else {
	errorStream << "ERROR: Failed to configure database, invalid ID found of FaultState ("
		    << id << ") for AllowedClass (" <<  (*it).second->id << ")";
	throw(DbException(errorStream.str()));
      }
    }
  }
}

void MpsDb::configureDeviceInputs() {
  std::stringstream errorStream;
  // Assign DeviceInputs to it's DigitalDevices, and find the Channel
  // the device is connected to
  for (DbDeviceInputMap::iterator it = deviceInputs->begin(); 
       it != deviceInputs->end(); ++it) {
    int id = (*it).second->digitalDeviceId;

    DbDigitalDeviceMap::iterator deviceIt = digitalDevices->find(id);
    if (deviceIt == digitalDevices->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for DigitalDevice ("
		  << id << ") for DeviceInput (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }

    // Create a map to hold deviceInput for the digitalDevice
    if (!(*deviceIt).second->inputDevices) {
      DbDeviceInputMap *deviceInputs = new DbDeviceInputMap();
      (*deviceIt).second->inputDevices = DbDeviceInputMapPtr(deviceInputs);
    }
    (*deviceIt).second->inputDevices->insert(std::pair<int, DbDeviceInputPtr>((*it).second->id,
									      (*it).second));

    int channelId = (*it).second->channelId;
    DbChannelMap::iterator channelIt = digitalChannels->find(channelId);
    if (channelIt == digitalChannels->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for Channel ("
		  << channelId << ") for DeviceInput (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    (*it).second->channel = (*channelIt).second;
  }
}

void MpsDb::configureFaultInputs() {
  std::stringstream errorStream;
  // Assign DigitalDevice to FaultInputs
  for (DbFaultInputMap::iterator it = faultInputs->begin();
       it != faultInputs->end(); ++it) {
    int id = (*it).second->deviceId;

    DbDigitalDeviceMap::iterator deviceIt = digitalDevices->find(id);
    if (deviceIt == digitalDevices->end()) {
      errorStream << "ERROR: Failed to find DigitalDevice (" << id
		  << ") for FaultInput (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    (*it).second->digitalDevice = (*deviceIt).second;
  }

  // Assing fault inputs to faults
  for (DbFaultInputMap::iterator it = faultInputs->begin();
       it != faultInputs->end(); ++it) {
    int id = (*it).second->faultId;

    DbFaultMap::iterator faultIt = faults->find(id);
    if (faultIt == faults->end()) {
      errorStream <<  "ERROR: Failed to configure database, invalid ID found for Fault ("
		  << id << ") for FaultInput (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }

    // Create a map to hold faultInputs for the fault
    if (!(*faultIt).second->faultInputs) {
      DbFaultInputMap *faultInputs = new DbFaultInputMap();
      (*faultIt).second->faultInputs = DbFaultInputMapPtr(faultInputs);
    }
    (*faultIt).second->faultInputs->insert(std::pair<int, DbFaultInputPtr>((*it).second->id,
									   (*it).second));
  }
}

void MpsDb::configureThresholdFaults() {
  std::stringstream errorStream;
  // Assign AnalogDevice to each ThresholdFault
  for (DbThresholdFaultMap::iterator it = thresholdFaults->begin();
       it != thresholdFaults->end(); ++it) {
    int id = (*it).second->analogDeviceId;
    DbAnalogDeviceMap::iterator analogIt = analogDevices->find(id);
    if (analogIt == analogDevices->end()) {
      errorStream << "ERROR: Failed to configure database, invalid AnalogDevice ("
		  << id << ") for ThresholdFault (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }

    (*it).second->analogDevice = (*analogIt).second;
  }

  // Assign ThresholdValue to each ThresholdFault
  for (DbThresholdFaultMap::iterator it = thresholdFaults->begin();
       it != thresholdFaults->end(); ++it) {
    int id = (*it).second->thresholdValueId;
    DbThresholdValueMap::iterator thresholdIt = thresholdValues->find(id);
    if (thresholdIt == thresholdValues->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ThresholdValue ("
		  << id << ") for ThresholdFault (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }

    (*it).second->thresholdValue = (*thresholdIt).second;
  }
}

void MpsDb::configureDigitalFaultStates() {
  std::stringstream errorStream;
  // Assign all DigitalFaultStates to a Fault, and also
  // find the DeviceState and save a reference in the DigitalFaultState
  for (DbDigitalFaultStateMap::iterator it = digitalFaultStates->begin();
       it != digitalFaultStates->end(); ++it) {
    int id = (*it).second->faultId;

    DbFaultMap::iterator faultIt = faults->find(id);
    if (faultIt == faults->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for Fault ("
		  << id << ") for DigitalFaultState (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }

    // Create a map to hold faultInputs for the fault
    if (!(*faultIt).second->digitalFaultStates) {
      DbDigitalFaultStateMap *digFaultStates = new DbDigitalFaultStateMap();
      (*faultIt).second->digitalFaultStates = DbDigitalFaultStateMapPtr(digFaultStates);
    }
    (*faultIt).second->digitalFaultStates->insert(std::pair<int, DbDigitalFaultStatePtr>((*it).second->id,
											 (*it).second));

    // If this DigitalFault is the default, then assign it to the fault:
    if ((*it).second->defaultState && !((*faultIt).second->defaultDigitalFaultState)) {
      (*faultIt).second->defaultDigitalFaultState = (*it).second;
    }

    // DevicState
    id = (*it).second->deviceStateId;
    DbDeviceStateMap::iterator deviceStateIt = deviceStates->find(id);
    if (deviceStateIt == deviceStates->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for DeviceState ("
		  << id << ") for DigitalFaultState (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    (*it).second->deviceState = (*deviceStateIt).second;
  }

  
}

void MpsDb::configureThresholdValues() {
  std::stringstream errorStream;
  // Assign list of ThresholdValues to each ThresholdValueMap
  for (DbThresholdValueMap::iterator it = thresholdValues->begin();
       it != thresholdValues->end(); ++it) {
    int thresholdMapId = (*it).second->thresholdValueMapId;

    DbThresholdValueMapEntryMap::iterator thresholdValueMapEntry =
      thresholdValueMaps->find(thresholdMapId);
    if (thresholdValueMapEntry != thresholdValueMaps->end()) {
      // Create a map to hold ThresholdValues for the ThresholdMap
      if (!(*thresholdValueMapEntry).second->thresholdValues) {
	DbThresholdValueMap *thresholdValueMap = new DbThresholdValueMap();
	(*thresholdValueMapEntry).second->thresholdValues = DbThresholdValueMapPtr(thresholdValueMap);
      }
      (*thresholdValueMapEntry).second->thresholdValues->
	insert(std::pair<int, DbThresholdValuePtr>((*it).second->id, (*it).second));      
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid thresholdValueMapId ("
		  << thresholdMapId << ") for thresholdValue (" <<  (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
  }
}

void MpsDb::configureAnalogDeviceTypes() {
  std::stringstream errorStream;
  // Assign the ThresholdValueMap to each AnalogDeviceType
  for (DbAnalogDeviceTypeMap::iterator it = analogDeviceTypes->begin();
       it != analogDeviceTypes->end(); ++it) {
    int thresholdMapId = (*it).second->thresholdValueMapId;

    DbThresholdValueMapEntryMap::iterator thresholdValueMapEntry =
      thresholdValueMaps->find(thresholdMapId);
    if (thresholdValueMapEntry != thresholdValueMaps->end()) {
      (*it).second->thresholdMapEntry = (*thresholdValueMapEntry).second;
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid thresholdValueMapId ("
		  << thresholdMapId << ") for AnalogDeviceType (" <<  (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
  }
}

void MpsDb::configureAnalogDevices() {
  std::stringstream errorStream;
  // Assign the AnalogDeviceType to each AnalogDevice
  for (DbAnalogDeviceMap::iterator it = analogDevices->begin();
       it != analogDevices->end(); ++it) {
    int typeId = (*it).second->analogDeviceTypeId;

    DbAnalogDeviceTypeMap::iterator analogDeviceType = analogDeviceTypes->find(typeId);
    if (analogDeviceType != analogDeviceTypes->end()) {
      (*it).second->analogDeviceType = (*analogDeviceType).second;
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid analogDeviceTypeId ("
		  << typeId << ") for AnalogDevice (" <<  (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    /*
    int channelId = (*it).second->channelId;
    DbChannelMap::iterator channelIt = analogChannels->find(channelId);
    if (channelIt == analogChannels->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for Channel ("
		  << channelId << ") for AnalogDevice (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    (*it).second->channel = (*channelIt).second;
    */
  }
}

void MpsDb::configureThresholdFaultStates() {
  std::stringstream errorStream;

  // Assign ThresholdFault to ThresholdFaultStates
  for (DbThresholdFaultStateMap::iterator it = thresholdFaultStates->begin();
       it != thresholdFaultStates->end(); ++it) {
    int thresholdFaultId = (*it).second->thresholdFaultId;

    DbThresholdFaultMap::iterator thresholdFault = thresholdFaults->find(thresholdFaultId);
    if (thresholdFault != thresholdFaults->end()) {
      (*it).second->thresholdFault = (*thresholdFault).second;
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid thresholdFaultId ("
		  << thresholdFaultId << ") for ThresholdFaultState (" <<  (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
  }
}

void MpsDb::configureIgnoreConditions() {
  std::stringstream errorStream;

  // Loop through the IgnoreConditions, and assign to the Condition
  for (DbIgnoreConditionMap::iterator ignoreCondition = ignoreConditions->begin();
       ignoreCondition != ignoreConditions->end(); ++ignoreCondition) {
    uint32_t conditionId = (*ignoreCondition).second->conditionId;

    DbConditionMap::iterator condition = conditions->find(conditionId);
    if (condition != conditions->end()) {
      // Create a map to hold IgnoreConditions
      if (!(*condition).second->ignoreConditions) {
	DbIgnoreConditionMap *ignoreConditionMap = new DbIgnoreConditionMap();
	(*condition).second->ignoreConditions = DbIgnoreConditionMapPtr(ignoreConditionMap);
      }
      (*condition).second->ignoreConditions->
	insert(std::pair<int, DbIgnoreConditionPtr>((*ignoreCondition).second->id, (*ignoreCondition).second));      
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid conditionId ("
		  << conditionId << ") for ignoreCondition (" <<  (*ignoreCondition).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    
    // Find if the ignored fault state is digital or analog
    uint32_t faultStateId = (*ignoreCondition).second->faultStateId;

    DbDigitalFaultStateMap::iterator digFaultIt = digitalFaultStates->find(faultStateId);
    if (digFaultIt != digitalFaultStates->end()) {
      (*ignoreCondition).second->digitalFaultState = (*digFaultIt).second;
    }
    else {
      DbThresholdFaultStateMap::iterator thresFaultIt = thresholdFaultStates->find(faultStateId);
      if (thresFaultIt != thresholdFaultStates->end()) {
	(*ignoreCondition).second->analogFaultState = (*thresFaultIt).second;
      }
      else {
	errorStream << "ERROR: Failed to configure database, invalid ID found of FaultState ("
		    << faultStateId << ") for IgnoreCondition (" <<  (*ignoreCondition).second->id << ")";
	throw(DbException(errorStream.str()));
      }
    }
  }

  // Loop through the ConditionInputs, and assign to the Condition
  for (DbConditionInputMap::iterator conditionInput = conditionInputs->begin();
       conditionInput != conditionInputs->end(); ++conditionInput) {
    uint32_t conditionId = (*conditionInput).second->conditionId;

    DbConditionMap::iterator condition = conditions->find(conditionId);
    if (condition != conditions->end()) {
      // Create a map to hold ConditionInputs
      if (!(*condition).second->conditionInputs) {
	DbConditionInputMap *conditionInputMap = new DbConditionInputMap();
	(*condition).second->conditionInputs = DbConditionInputMapPtr(conditionInputMap);
      }
      (*condition).second->conditionInputs->
	insert(std::pair<int, DbConditionInputPtr>((*conditionInput).second->id, (*conditionInput).second));      
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid conditionId ("
		  << conditionId << ") for conditionInput (" <<  (*conditionInput).second->id << ")";
      throw(DbException(errorStream.str()));
    }

    // Find if the ignored fault state is digital or analog
    uint32_t faultStateId = (*conditionInput).second->faultStateId;

    DbDigitalFaultStateMap::iterator digFaultIt = digitalFaultStates->find(faultStateId);
    if (digFaultIt != digitalFaultStates->end()) {
      (*conditionInput).second->digitalFaultState = (*digFaultIt).second;
    }
    else {
      DbThresholdFaultStateMap::iterator thresFaultIt = thresholdFaultStates->find(faultStateId);
      if (thresFaultIt != thresholdFaultStates->end()) {
	(*conditionInput).second->analogFaultState = (*thresFaultIt).second;
      }
      else {
	errorStream << "ERROR: Failed to configure database, invalid ID found of FaultState ("
		    << faultStateId << ") for ConditionInput (" <<  (*conditionInput).second->id << ")";
	throw(DbException(errorStream.str()));
      }
    }
  }
}

/**
 * After the YAML database file is loaded this method must be called to resolve
 * references between tables. In the YAML table entries there is a reference to
 * another table using a 'foreign key'. The configure*() methods look for the 
 * actual item referenced by the foreign key and saves a pointer to the
 * referenced element for direct access. This allows the engine to evaluate
 * faults without having to search for entries.
 */
void MpsDb::configure() {
  configureAllowedClasses();
  configureDeviceInputs();
  configureFaultInputs();
  configureThresholdFaults();
  configureDigitalFaultStates();
  configureThresholdValues();
  configureAnalogDeviceTypes();
  configureAnalogDevices();
  configureThresholdFaultStates();
  configureIgnoreConditions();
}

void MpsDb::setName(std::string yamlFileName) {
  name = yamlFileName;
}

/**
 * The MPS database YAML file is composed of several "tables"/"set of entries".
 * This method opens each table as a YAML::Node and loads all entry contents
 * into the MpsDb maps. Each maps uses the MPS database id as key.
 */
int MpsDb::load(std::string yamlFileName) {
  std::stringstream errorStream;
  YAML::Node doc;
  std::vector<YAML::Node> nodes;

  LOG_TRACE("DATABASE", "Loading YAML from file " << yamlFileName);
  try {
    nodes = YAML::LoadAllFromFile(yamlFileName);
  } catch (...) {
    errorStream << "ERROR: Failed to load YAML file ("
		<< yamlFileName << ")";
    throw(DbException(errorStream.str()));
  }

  LOG_TRACE("DATABASE", "Parsing YAML");
  for (std::vector<YAML::Node>::iterator node = nodes.begin();
       node != nodes.end(); ++node) {
    std::string s = YAML::Dump(*node);
    size_t found = s.find(":");
    if (found == std::string::npos) {
      errorStream << "ERROR: Can't find Node name in YAML file ("
		  << yamlFileName << ")";
      throw(DbException(errorStream.str()));
    }
    std::string nodeName = s.substr(0, found);

    LOG_TRACE("DATABASE", "Parsing " << nodeName);

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
    } else if (nodeName == "Condition") {
      conditions = (*node).as<DbConditionMapPtr>();
    } else if (nodeName == "IgnoreCondition") {
      ignoreConditions = (*node).as<DbIgnoreConditionMapPtr>();
    } else if (nodeName == "ConditionInput") {
      conditionInputs = (*node).as<DbConditionInputMapPtr>();
    } else {
      errorStream << "ERROR: Unknown YAML node name ("
		  << nodeName << ")";
      throw(DbException(errorStream.str()));
    }
  }

  setName(yamlFileName);

  return 0;
}

/**
 * Print out the digital/analog inputs (DbFaultInput and DbAnalogDevices)
 */
void MpsDb::showFaults() {
  std::cout << "+-------------------------------------------------------" << std::endl;
  std::cout << "| Digital Faults: " << std::endl;
  std::cout << "+-------------------------------------------------------" << std::endl;
  for (DbFaultMap::iterator fault = faults->begin(); 
       fault != faults->end(); ++fault) {
    showFault((*fault).second);
  }

  std::cout << "+-------------------------------------------------------" << std::endl;
  std::cout << "| Analog Faults: " << std::endl;
  std::cout << "+-------------------------------------------------------" << std::endl;
  for (DbThresholdFaultMap::iterator fault = thresholdFaults->begin(); 
       fault != thresholdFaults->end(); ++fault) {
    showThresholdFault((*fault).second);
  }
  std::cout << "+-------------------------------------------------------" << std::endl;
}

void MpsDb::showFault(DbFaultPtr fault) {
  std::cout << "| " << fault->name << std::endl;
  if (!fault->faultInputs) {
    std::cout << "| - WARNING: No inputs to this fault" << std::endl;
  }
  else {
    for (DbFaultInputMap::iterator input = fault->faultInputs->begin();
	 input != fault->faultInputs->end(); ++input) {
      int digitalDeviceId = (*input).second->deviceId;
      DbDigitalDeviceMap::iterator digitalDevice = digitalDevices->find(digitalDeviceId);
      if (digitalDevice == digitalDevices->end()) {
	std::cout << "| - WARNING: No digital devices for this fault!";
      }
      else {
	for (DbDeviceInputMap::iterator devInput =
	       (*digitalDevice).second->inputDevices->begin();
	     devInput != (*digitalDevice).second->inputDevices->end(); ++devInput) {
	  std::cout << "| - Input[" << (*devInput).second->id
		    << "], Position[" << (*devInput).second->bitPosition
		    << "], Value[" << (*devInput).second->value
		    << "], Bypass[";
      
	  if (!(*devInput).second->bypass) {
	    std::cout << "WARNING: NO BYPASS INFO]";
	  }
	  else {
	    if ((*devInput).second->bypass->status == BYPASS_VALID) {
	      std::cout << "VALID]";
	    }
	    else {
	      std::cout << "EXPIRED]";
	    }
	  }
	  std::cout << std::endl;
	}
      }
    }
  }
}

void MpsDb::showThresholdFault(DbThresholdFaultPtr fault) {
  std::cout << "| " << fault->name << std::endl;
  if (!fault->analogDevice) {
    std::cout << "| - WARNING: No inputs to this fault!" << std::endl;
  }
  else {
    std::cout << "| - Input[" << fault->analogDevice->id
	      << "], Value[" << fault->analogDevice->value
	      << "], Type[" << fault->analogDevice->analogDeviceType->name
	      << "], Bypass[";
    
    if (!fault->analogDevice->bypass) {
      std::cout << "WARNING: NO BYPASS INFO]";
    }
    else {
      if (fault->analogDevice->bypass->status == BYPASS_VALID) {
	std::cout << "VALID]";
      }
      else {
	std::cout << "EXPIRED]";
      }
    }
    std::cout << std::endl;
  }
}
