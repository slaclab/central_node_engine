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
  LOG_TRACE("DATABASE", "Configure: AllowedClasses");
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

  // Assign AllowedClasses to FaultState or ThresholdFaultState
  // There are multiple AllowedClasses for each Fault, one per MitigationDevice
  for (DbAllowedClassMap::iterator it = allowedClasses->begin();
       it != allowedClasses->end(); ++it) {
    int id = (*it).second->faultStateId;

    DbFaultStateMap::iterator digFaultIt = faultStates->find(id);
    if (digFaultIt != faultStates->end()) {
      // Create a map to hold deviceInput for the digitalDevice
      if (!(*digFaultIt).second->allowedClasses) {
	DbAllowedClassMap *faultAllowedClasses = new DbAllowedClassMap();
	(*digFaultIt).second->allowedClasses = DbAllowedClassMapPtr(faultAllowedClasses);
      }
      (*digFaultIt).second->allowedClasses->insert(std::pair<int, DbAllowedClassPtr>((*it).second->id,
										     (*it).second));      
    }
  }
}

void MpsDb::configureDeviceInputs() {
  LOG_TRACE("DATABASE", "Configure: DeviceInputs");
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
  LOG_TRACE("DATABASE", "Configure: FaultInputs");
  std::stringstream errorStream;
  // Assign DigitalDevice to FaultInputs
  for (DbFaultInputMap::iterator it = faultInputs->begin();
       it != faultInputs->end(); ++it) {
    int id = (*it).second->deviceId;

    DbDigitalDeviceMap::iterator deviceIt = digitalDevices->find(id);
    if (deviceIt == digitalDevices->end()) {
      DbAnalogDeviceMap::iterator aDeviceIt = analogDevices->find(id);
      if (aDeviceIt == analogDevices->end()) {
	errorStream << "ERROR: Failed to find DigitalDevice/AnalogDevice (" << id
		    << ") for FaultInput (" << (*it).second->id << ")";
	throw(DbException(errorStream.str()));
      }
      else {
	(*it).second->analogDevice = (*aDeviceIt).second;
      }
    }
    else {
      (*it).second->digitalDevice = (*deviceIt).second;
    }
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

void MpsDb::configureFaultStates() {
  LOG_TRACE("DATABASE", "Configure: FaultStates");
 std::stringstream errorStream;
  // Assign all FaultStates to a Fault, and also
  // find the DeviceState and save a reference in the FaultState
  for (DbFaultStateMap::iterator it = faultStates->begin();
       it != faultStates->end(); ++it) {
    int id = (*it).second->faultId;

    DbFaultMap::iterator faultIt = faults->find(id);
    if (faultIt == faults->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for Fault ("
		  << id << ") for FaultState (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }

    // Create a map to hold faultInputs for the fault
    if (!(*faultIt).second->faultStates) {
      DbFaultStateMap *digFaultStates = new DbFaultStateMap();
      (*faultIt).second->faultStates = DbFaultStateMapPtr(digFaultStates);
    }
    (*faultIt).second->faultStates->insert(std::pair<int, DbFaultStatePtr>((*it).second->id,
											 (*it).second));

    // If this DigitalFault is the default, then assign it to the fault:
    if ((*it).second->defaultState && !((*faultIt).second->defaultFaultState)) {
      (*faultIt).second->defaultFaultState = (*it).second;
    }

    // DevicState
    id = (*it).second->deviceStateId;
    DbDeviceStateMap::iterator deviceStateIt = deviceStates->find(id);
    if (deviceStateIt == deviceStates->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for DeviceState ("
		  << id << ") for FaultState (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    (*it).second->deviceState = (*deviceStateIt).second;
  }

  
}


void MpsDb::configureAnalogDevices() {
  LOG_TRACE("DATABASE", "Configure: AnalogDevices");
  std::stringstream errorStream;
  // Assign the AnalogDeviceType to each AnalogDevice
  for (DbAnalogDeviceMap::iterator it = analogDevices->begin();
       it != analogDevices->end(); ++it) {
    int typeId = (*it).second->deviceTypeId;

    DbDeviceTypeMap::iterator deviceType = deviceTypes->find(typeId);
    if (deviceType != deviceTypes->end()) {
      (*it).second->deviceType = (*deviceType).second;
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid deviceTypeId ("
		  << typeId << ") for AnalogDevice (" <<  (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    
    int channelId = (*it).second->channelId;
    DbChannelMap::iterator channelIt = analogChannels->find(channelId);
    if (channelIt == analogChannels->end()) {
      errorStream << "ERROR: Failed to configure database, invalid ID found for Channel ("
		  << channelId << ") for AnalogDevice (" << (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
    (*it).second->channel = (*channelIt).second;
  }
}

void MpsDb::configureIgnoreConditions() {
  LOG_TRACE("DATABASE", "Configure: IgnoreConditions");
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

    DbFaultStateMap::iterator digFaultIt = faultStates->find(faultStateId);
    if (digFaultIt != faultStates->end()) {
      (*ignoreCondition).second->faultState = (*digFaultIt).second;
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid ID found of FaultState ("
		  << faultStateId << ") for IgnoreCondition (" <<  (*ignoreCondition).second->id << ")";
      throw(DbException(errorStream.str()));
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

    DbFaultStateMap::iterator digFaultIt = faultStates->find(faultStateId);
    if (digFaultIt != faultStates->end()) {
      (*conditionInput).second->faultState = (*digFaultIt).second;
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid ID found of FaultState ("
		  << faultStateId << ") for ConditionInput (" <<  (*conditionInput).second->id << ")";
      throw(DbException(errorStream.str()));
    }
  }
}

/**
 * Configure each application card, setting its map of digital or analog
 * inputs, and setting up the location within the fast configuration memory.
 */
void MpsDb::configureApplicationCards() {
  std::stringstream errorStream;

  //  char fastConfigurationBuffer[NUM_APPLICATIONS * APPLICATION_CONFIG_BUFFER_SIZE];
  char *configBuffer = 0;
  for (DbApplicationCardMap::iterator applicationCard = applicationCards->begin();
       applicationCard != applicationCards->end(); ++applicationCard) {
    configBuffer = fastConfigurationBuffer + (*applicationCard).second->globalId *
      APPLICATION_CONFIG_BUFFER_SIZE;
    (*applicationCard).second->applicationBuffer = 
      reinterpret_cast<ApplicationConfigBufferBitSet *>(configBuffer);
  }

  // Get all devices for each application card, starting with the digital devices
  for (DbDigitalDeviceMap::iterator digitalDevice = digitalDevices->begin();
       digitalDevice != digitalDevices->end(); ++digitalDevice) {
    DbApplicationCardMap::iterator applicationCard =
      applicationCards->find((*digitalDevice).second->cardId);
    if (applicationCard != applicationCards->end()) {
      // Alloc a new map for digital devices if one is not there yet
      if (!(*applicationCard).second->digitalDevices) {
	DbDigitalDeviceMap *digitalDevices = new DbDigitalDeviceMap();
	(*applicationCard).second->digitalDevices = DbDigitalDeviceMapPtr(digitalDevices);
      }
      // Once the map is there, add the digital device
      (*applicationCard).second->digitalDevices->
	insert(std::pair<int, DbDigitalDevicePtr>((*digitalDevice).second->id,
						  (*digitalDevice).second));
    }
  }

  // Do the same for analog devices
  for (DbAnalogDeviceMap::iterator analogDevice = analogDevices->begin();
       analogDevice != analogDevices->end(); ++analogDevice) {
    DbApplicationCardMap::iterator applicationCard =
      applicationCards->find((*analogDevice).second->cardId);
    if (applicationCard != applicationCards->end()) {
      // Alloc a new map for analog devices if one is not there yet
      if ((*applicationCard).second->digitalDevices) {
	errorStream << "ERROR: Found ApplicationCard with digital AND analog devices,"
		    << " can't handle that (cardId="
		    << (*analogDevice).second->cardId << ")";
	throw(DbException(errorStream.str()));
      }
      if (!(*applicationCard).second->analogDevices) {
	DbAnalogDeviceMap *analogDevices = new DbAnalogDeviceMap();
	(*applicationCard).second->analogDevices = DbAnalogDeviceMapPtr(analogDevices);
      }
      // Once the map is there, add the analog device
      (*applicationCard).second->analogDevices->
	insert(std::pair<int, DbAnalogDevicePtr>((*analogDevice).second->id,
						  (*analogDevice).second));
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
  configureFaultStates();
  configureAnalogDevices();
  configureIgnoreConditions();
  configureApplicationCards();
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

    LOG_TRACE("DATABASE", "Parsing \"" << nodeName << "\"");

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
    } else if (nodeName == "FaultState") {
      faultStates = (*node).as<DbFaultStateMapPtr>();
    } else if (nodeName == "AnalogDevice") {
      analogDevices = (*node).as<DbAnalogDeviceMapPtr>();
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

  // Zero out the buffer that holds the firmware configuration
  memset(fastConfigurationBuffer, 0, NUM_APPLICATIONS * APPLICATION_CONFIG_BUFFER_SIZE); 

  return 0;
}

/**
 * Print out the digital/analog inputs (DbFaultInput and DbAnalogDevices)
 */
void MpsDb::showFaults() {
  std::cout << "+-------------------------------------------------------" << std::endl;
  std::cout << "| Faults: " << std::endl;
  std::cout << "+-------------------------------------------------------" << std::endl;
  for (DbFaultMap::iterator fault = faults->begin(); 
       fault != faults->end(); ++fault) {
    showFault((*fault).second);
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
      int deviceId = (*input).second->deviceId;
      DbDigitalDeviceMap::iterator digitalDevice = digitalDevices->find(deviceId);
      if (digitalDevice == digitalDevices->end()) {
	// If no inputs, then this could be an analog device
	DbAnalogDeviceMap::iterator analogDevice = analogDevices->find(deviceId);
	if (analogDevice == analogDevices->end()) {
	  std::cout << "| - WARNING: No digital/analog devices for this fault!";
	}
	else {
	  std::cout << "| - Input[" << (*analogDevice).second->name
		    << "], Bypass[";
	  if (!(*analogDevice).second->bypass) {
	    std::cout << "WARNING: NO BYPASS INFO]";
	  }
	  else {
	    if ((*analogDevice).second->bypass->status == BYPASS_VALID) {
	      std::cout << "VALID]";
	    }
	    else {
	      std::cout << "EXPIRED]";
	    }
	  }
	  std::cout << std::endl;
	}
      }
      else {
	for (DbDeviceInputMap::iterator devInput =
	       (*digitalDevice).second->inputDevices->begin();
	     devInput != (*digitalDevice).second->inputDevices->end(); ++devInput) {
	  std::cout << "| - Input[" << (*devInput).second->id
		    << "], Position[" << (*devInput).second->bitPosition
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

