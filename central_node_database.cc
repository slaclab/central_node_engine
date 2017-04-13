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

/**
 * 
 */
void DbApplicationCard::writeConfiguration() {
  if (digitalDevices) {
    writeDigitalConfiguration();
  }
  else if (analogDevices) {
    writeAnalogConfiguration();
  }
  else {
    throw(DbException("Can't configure application card - no devices configured"));
  }
}

// Digital input configuration (total size = 1344 bits):
//
//   +------------> Expected digital input state 
//   |   +--------> 16-bit destination mask
//   |   |    +---> 4-bit encoded power class
//   |   |    |
// +---+----+---+---        +---+----+---+---+----+---+
// | 1 | 16 | 4 |    ...    | 1 | 16 | 4 | 1 | 16 | 4 |
// +---+----+---+---     ---+---+----+---+---+----+---+
// |    M63     |    ...    |     M1     |     M0     |
//
// where Mxx is the mask for bit xx
void DbApplicationCard::writeDigitalConfiguration() {
  // First set all bits to zero
  applicationBuffer->reset();
  
  std::stringstream errorStream;
  for (DbDigitalDeviceMap::iterator digitalDevice = digitalDevices->begin();
       digitalDevice != digitalDevices->end(); ++digitalDevice) {
    // Only configure firmware for devices/faults that require fast evaluation
    if ((*digitalDevice).second->evaluation == FAST_EVALUATION) {
      if ((*digitalDevice).second->inputDevices->size() == 1) {
	DbDeviceInputMap::iterator deviceInput = (*digitalDevice).second->inputDevices->begin();

	// Write the ExpectedState (index 20)
	int channelNumber = (*deviceInput).second->channel->number;
	int channelOffset = channelNumber * DIGITAL_CHANNEL_CONFIG_SIZE;
	int offset = DIGITAL_CHANNEL_EXPECTED_STATE_OFFSET;
	applicationBuffer->set(channelOffset + offset, (*digitalDevice).second->fastExpectedState);

	// Write the destination mask (index 4 through 19)
	offset = DIGITAL_CHANNEL_DESTINATION_MASK_OFFSET;
	for (int i = 0; i < DESTINATION_MASK_BIT_SIZE; ++i) {
	  applicationBuffer->set(channelOffset + offset + i,
				 ((*digitalDevice).second->fastDestinationMask >> i) & 0x01);
	}

	// Write the beam power class (index 0 through 3)
	offset = DIGITAL_CHANNEL_POWER_CLASS_OFFSET;
	for (int i = 0; i < POWER_CLASS_BIT_SIZE; ++i) {
	  applicationBuffer->set(channelOffset + offset + i,
				 ((*digitalDevice).second->fastPowerClass >> i) & 0x01);
	}
      }
      else {
	errorStream << "ERROR: DigitalDevice configured with FAST evaluation must have one input only."
		    << " Found " << (*digitalDevice).second->inputDevices->size() << " inputs for "
		    << "device " << (*digitalDevice).second->name;
	throw(DbException(errorStream.str())); 
      }
    }
  }
  // Print bitset in string format
  //  std::cout << *applicationBuffer << std::endl;
}

// Analog input configuration (total size = 1152 bits):
//
//   +------------> 16-bit destination mask
//   |   
//   |                        +---> 4-bit encoded power class
//   |                        |
// +----+----+---   ---+----+----+---+---   ---+---+---+
// | 16 | 16 |   ...   | 16 | 4  | 4 |   ...   | 4 | 4 |
// +----+----+---   ---+----+----+---+---   ---+---+---+
// |B23 |B22 |   ...   | B0 |M191|M190   ...   | M1| M0|
//
// Bxx are the 16-bit destination masks for each 
// 4 integrators of each of the 6 channels:
// 6 * 4 = 24 (B0-B23)
// 
// Power classes (M0-M191):
// 4 integrators per channel (BPM has only 3 - X, Y and TMIT)
// 8 comparators for each integrator:
// 8 * 4 * 6 = 192 (M0 through M191)
void DbApplicationCard::writeAnalogConfiguration() {
  // First set all bits to zero
  applicationBuffer->reset();
  
  std::stringstream errorStream;

  // Loop through all Analog Devices within this Application
  for (DbAnalogDeviceMap::iterator analogDevice = analogDevices->begin();
       analogDevice != analogDevices->end(); ++analogDevice) {
    // Only configure firmware for devices/faults that require fast evaluation
    if ((*analogDevice).second->evaluation == FAST_EVALUATION) {
      LOG_TRACE("DATABASE", "AnalogConfig: " << (*analogDevice).second->name);
      std::cout << (*analogDevice).second << std::endl;
      int channelNumber = (*analogDevice).second->channel->number;

      // Write power classes for each threshold bit
      uint32_t powerClassOffset = channelNumber *
	ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL * POWER_CLASS_BIT_SIZE;
      for (int i = 0; i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL *
	     ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
	for (int j = 0; j < POWER_CLASS_BIT_SIZE; ++j) {
	  applicationBuffer->set(powerClassOffset + j,
				 ((*analogDevice).second->fastPowerClass[i] >> j) & 0x01);
	}
	powerClassOffset += POWER_CLASS_BIT_SIZE;
      }
      
      // Write the destination mask for each integrator
      int offset = ANALOG_CHANNEL_DESTINATION_MASK_BASE +
	channelNumber * ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL * DESTINATION_MASK_BIT_SIZE;
      for (int i = 0; i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL; ++i) {
	for (int j = 0; j < DESTINATION_MASK_BIT_SIZE; ++j) {
	  applicationBuffer->set(offset + j,
				 ((*analogDevice).second->fastDestinationMask[i] >> j) & 0x01);
	}
	offset += DESTINATION_MASK_BIT_SIZE;
      }
    }
  }
  // Print bitset in string format
  std::cout << *applicationBuffer << std::endl;
}

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

void MpsDb::configureDeviceTypes() {
  // Compile a list of DeviceStates and assign to the proper DeviceType
  for (DbDeviceStateMap::iterator it = deviceStates->begin(); 
       it != deviceStates->end(); ++it) {
    int id = (*it).second->deviceTypeId;

    DbDeviceTypeMap::iterator deviceType = deviceTypes->find(id);
    if (deviceType != deviceTypes->end()) {
      // Create a map to hold deviceStates for the deviceType
      if (!(*deviceType).second->deviceStates) {
	DbDeviceStateMap *deviceStates = new DbDeviceStateMap();
	(*deviceType).second->deviceStates = DbDeviceStateMapPtr(deviceStates);
      }
      (*deviceType).second->deviceStates->insert(std::pair<int, DbDeviceStatePtr>((*it).second->id,
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

  // Set the DbDeviceType for all DbDigitaDevices
  for (DbDigitalDeviceMap::iterator it = digitalDevices->begin();
       it != digitalDevices->end(); ++it) {
    int typeId = (*it).second->deviceTypeId;

    DbDeviceTypeMap::iterator deviceType = deviceTypes->find(typeId);
    if (deviceType != deviceTypes->end()) {
      (*it).second->deviceType = (*deviceType).second;
    }
    else {
      errorStream << "ERROR: Failed to configure database, invalid deviceTypeId ("
		  << typeId << ") for DigitalDevice (" <<  (*it).second->id << ")";
      throw(DbException(errorStream.str()));
    }
  }

}

void MpsDb::configureFaultInputs() {
  LOG_TRACE("DATABASE", "Configure: FaultInputs");
  std::stringstream errorStream;
  // Assign DigitalDevice to FaultInputs
  for (DbFaultInputMap::iterator it = faultInputs->begin();
       it != faultInputs->end(); ++it) {
    int id = (*it).second->deviceId;

    // Find the DbDigitalDevice or DbAnalogDevice referenced by the FaultInput
    DbDigitalDeviceMap::iterator deviceIt = digitalDevices->find(id);
    if (deviceIt == digitalDevices->end()) {
      DbAnalogDeviceMap::iterator aDeviceIt = analogDevices->find(id);
      if (aDeviceIt == analogDevices->end()) {
	errorStream << "ERROR: Failed to find DigitalDevice/AnalogDevice (" << id
		    << ") for FaultInput (" << (*it).second->id << ")";
	throw(DbException(errorStream.str()));
      }
      // Found the DbAnalogDevice, configure it
      else {
	(*it).second->analogDevice = (*aDeviceIt).second;
	if ((*aDeviceIt).second->evaluation == FAST_EVALUATION) {
	  LOG_TRACE("DATABASE", "AnalogDevice " << (*aDeviceIt).second->name
		    << ": Fast Evaluation");
	  DbFaultMap::iterator faultIt = faults->find((*it).second->faultId);
	  if (faultIt == faults->end()) {
	    errorStream << "ERROR: Failed to find Fault (" << id
			<< ") for FaultInput (" << (*it).second->id << ")";
	    throw(DbException(errorStream.str()));
	  }
	  else {
	    if (!(*faultIt).second->faultStates) {
	      errorStream << "ERROR: No FaultStates found for Fault (" << (*faultIt).second->id
			  << ") for FaultInput (" << (*it).second->id << ")";
	      throw(DbException(errorStream.str()));
	    }
	    // There is one DbFaultState per threshold bit, for BPMs there are
	    // 24 bits (8 for X, 8 for Y and 8 for TMIT). Other analog devices
	    // have 32 bits (4 integrators).

	    // There is a unique power class for each threshold bit (DbFaultState)
	    // The destination masks for the thresholds for the same integrator
	    // must be and'ed together.
	    
	    for (int i = 0; i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL; ++i) {
	      (*aDeviceIt).second->fastDestinationMask[i] = 0;
	    }
	    
	    for (int i = 0;
		 i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL * ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
	      (*aDeviceIt).second->fastPowerClass[i] = 0xFF;
	    }

	    for (DbFaultStateMap::iterator faultState = (*faultIt).second->faultStates->begin();
		 faultState != (*faultIt).second->faultStates->end(); ++faultState) {
	      // The DbDeviceState value defines which integrator the fault belongs to.
	      // Bits 0 through 7 are for the first integrator, 8 through 15 for the second,
	      // and so on.
	      uint32_t value = (*faultState).second->deviceState->value;
	      int integratorIndex = 4;

	      if (value & 0x000000FF) integratorIndex = 0;
	      if (value & 0x0000FF00) integratorIndex = 1;
	      if (value & 0x00FF0000) integratorIndex = 2;
	      if (value & 0xFF000000) integratorIndex = 3;

	      int thresholdIndex = 0;
	      bool isSet = false;
	      while (!isSet) {
		if (value & 0x01) {
		  isSet = true;
		}
		else {
		  thresholdIndex++;
		  value >>= 1;
		}
		if (thresholdIndex >= sizeof(uint32_t) * 8) {
		  errorStream << "ERROR: Invalid threshold bit for Fault (" << (*faultIt).second->id
			      << "), FaultInput (" << (*it).second->id << "), DeviceState ("
			      << (*faultState).second->deviceState->id << ")";
		  throw(DbException(errorStream.str()));
		}
	      }

	      for (DbAllowedClassMap::iterator allowedClass =
		     (*faultState).second->allowedClasses->begin();
		   allowedClass != (*faultState).second->allowedClasses->end(); ++allowedClass) {
		(*aDeviceIt).second->fastDestinationMask[integratorIndex] |=
		  (*allowedClass).second->mitigationDevice->destinationMask;
		LOG_TRACE("DATABASE", "PowerClass: threshold index=" << thresholdIndex
			  << " power=" << (*allowedClass).second->beamClass->number);
		if ((*allowedClass).second->beamClass->number <
		    (*aDeviceIt).second->fastPowerClass[thresholdIndex]) {
		  (*aDeviceIt).second->fastPowerClass[thresholdIndex] =
		    (*allowedClass).second->beamClass->number;
		}
	      }
	    }
	    for (int i = 0;
		 i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL * ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
	      if ((*aDeviceIt).second->fastPowerClass[i] == 0xFF) {
		(*aDeviceIt).second->fastPowerClass[i] = 0;
	      }
	    }
	  }
	}
      }
    }
    else {
      (*it).second->digitalDevice = (*deviceIt).second;
      // If the DbDigitalDevice is set for fast evaluation, save a pointer to the 
      // DbFaultInput
      if ((*deviceIt).second->evaluation == FAST_EVALUATION) {
	DbFaultMap::iterator faultIt = faults->find((*it).second->faultId);
	if (faultIt == faults->end()) {
	  errorStream << "ERROR: Failed to find Fault (" << id
		      << ") for FaultInput (" << (*it).second->id << ")";
	  throw(DbException(errorStream.str()));
	}
	else {
	  if (!(*faultIt).second->faultStates) {
	    errorStream << "ERROR: No FaultStates found for Fault (" << (*faultIt).second->id
			<< ") for FaultInput (" << (*it).second->id << ")";
	    throw(DbException(errorStream.str()));
	  }
	  (*deviceIt).second->fastDestinationMask = 0;
	  (*deviceIt).second->fastPowerClass = 100;
	  (*deviceIt).second->fastExpectedState = 0;

	  if ((*faultIt).second->faultStates->size() != 1) {
	    errorStream << "ERROR: DigitalDevice configured with FAST evaluation must have one fault state only."
			<< " Found " << (*faultIt).second->faultStates->size() << " fault states for "
			<< "device " << (*deviceIt).second->name;
	    throw(DbException(errorStream.str())); 
	  }
	  DbFaultStateMap::iterator faultState = (*faultIt).second->faultStates->begin();

	  // Set the expected state as the oposite of the expected faulted value
	  // For example a faulted fast valve sets the digital input to high (0V, digital 0),
	  // the expected normal state is 1.
	  if (!(*faultState).second->deviceState->value) {
	    (*deviceIt).second->fastExpectedState = 1;
	  }

	  //	  DbAllowedClassMap::iterator allowedClass = (*faultState).second->allowedClasses->begin();
	  for (DbAllowedClassMap::iterator allowedClass = (*faultState).second->allowedClasses->begin();
	       allowedClass != (*faultState).second->allowedClasses->end(); ++allowedClass) {
	    (*deviceIt).second->fastDestinationMask |= (*allowedClass).second->mitigationDevice->destinationMask;
	    if ((*allowedClass).second->beamClass->number < (*deviceIt).second->fastPowerClass) {
	      (*deviceIt).second->fastPowerClass = (*allowedClass).second->beamClass->number;
	    }
	  }
	}
      }
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

    // DeviceState
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
  configureDeviceTypes();
  configureDeviceInputs();
  configureFaultStates();
  configureFaultInputs();
  configureAnalogDevices();
  configureIgnoreConditions();
  configureApplicationCards();
}

void MpsDb::writeFirmwareConfiguration() {
  for (DbApplicationCardMap::iterator card = applicationCards->begin();
       card != applicationCards->end(); ++card) {
    (*card).second->writeConfiguration();
  }
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

