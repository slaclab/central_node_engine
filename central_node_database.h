#ifndef CENTRAL_NODE_DATABASE_H
#define CENTRAL_NODE_DATABASE_H

#include <map>
#include <exception>
#include <iostream>
#include <bitset>
#include <central_node_exception.h>
#include <central_node_bypass.h>
#include <stdint.h>

#include <boost/shared_ptr.hpp>

using boost::shared_ptr;
using boost::weak_ptr;

const uint32_t SLOW_EVALUATION = 0;
const uint32_t FAST_EVALUATION = 1;

// Definition of number of applications and memory space for
// the hardware (fast) configuration
const uint32_t NUM_APPLICATIONS = 1024;
const uint32_t APPLICATION_CONFIG_BUFFER_SIZE = 2048; // in bytes

//
// Each application card has an array of bits for the fast firmware
// evaluation. The formats are different for digital and analog inputs
// 
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
//
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
// 
typedef std::bitset<APPLICATION_CONFIG_BUFFER_SIZE> ApplicationConfigBufferBitSet;

/**
 * DbException class
 * 
 * It is thrown by any Db* classes when there is a problem loading or
 * configuring the database.
 */
class DbException: public CentralNodeException {
 public:
  explicit DbException(const char* message) : CentralNodeException(message) {}
  explicit DbException(const std::string& message) : CentralNodeException(message) {}
  virtual ~DbException() throw () {}
};

/**
 * Simple database entry - has only an ID.
 */
class DbEntry {
 public:
  uint32_t id;

  DbEntry() : id(-1) {
  }

  std::ostream & operator<<(std::ostream &os) {
    os << " id[" << id << "];";
    return os;
  }
};

/**
 * DbCrate class
 * 
 * Example YAML entry:
 * Crate:
 * - id: '1'
 *   num_slots: '6'
 *   number: '1'
 *   shelf_number: '1'
 */
class DbCrate : public DbEntry {
 public:
  uint32_t number;
  uint32_t numSlots;
  uint32_t shelfNumber;
  
 DbCrate() : DbEntry(), number(-1), numSlots(-1), shelfNumber(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbCrate * const crate) {
    os << "id[" << crate->id << "]; " 
       << "number[" << crate->number << "]; "
       << "slots[" << crate->numSlots << "]; "
       << "shelf[" << crate->shelfNumber << "]";
    return os;
  }
};

typedef shared_ptr<DbCrate> DbCratePtr;
typedef std::map<uint32_t, DbCratePtr> DbCrateMap;
typedef shared_ptr<DbCrateMap> DbCrateMapPtr;

/**
 * ApplicationType:
 * - analog_channel_count: '3'
 *   analog_channel_size: '8'
 *   digital_channel_count: '4'
 *   digital_channel_size: '1'
 *   id: '1'
 *   name: Mixed Mode Link Node
 *   number: '0'
 */
class DbApplicationType : public DbEntry {
 public:
  uint32_t number;
  uint32_t analogChannelCount;
  uint32_t analogChannelSize;
  uint32_t digitalChannelCount;
  uint32_t digitalChannelSize;
  std::string description;

 DbApplicationType() : DbEntry(), number(-1),
    analogChannelCount(-1), analogChannelSize(-1),
    digitalChannelCount(-1), digitalChannelSize(-1),
    description("empty") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbApplicationType * const appType) {
    os << "id[" << appType->id << "]; " 
       << "number[" << appType->number << "]; "
       << "analogChannelCount[" << appType->analogChannelCount << "]; "
       << "analogChannelSize[" << appType->analogChannelSize << "]; "
       << "digitalChannelCount[" << appType->digitalChannelCount << "]; "
       << "digitalChannelSize[" << appType->digitalChannelSize << "]; "
       << "description[" << appType->description << "]";
    return os;
  }
};

typedef shared_ptr<DbApplicationType> DbApplicationTypePtr;
typedef std::map<uint32_t, DbApplicationTypePtr> DbApplicationTypeMap;
typedef shared_ptr<DbApplicationTypeMap> DbApplicationTypeMapPtr;

/**
 * DigitalChannel:
 * - card_id: '1'
 *   id: '1'
 *   number: '0'
 *
 * AnalogChannel:
 * - card_id: '1'
 *   id: '1'
 *   number: '0'
 */
class DbChannel : public DbEntry {
 public:
  uint32_t number;
  uint32_t cardId;
  std::string name;
  
 DbChannel() : DbEntry(), number(-1), cardId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbChannel * const channel) {
    os << "id[" << channel->id << "]; " 
       << "number[" << channel->number << "]; "
       << "cardId[" << channel->cardId << "]; " 
       << "name[" << channel->name << "]";
    return os;
  }
};

typedef shared_ptr<DbChannel> DbChannelPtr;
typedef std::map<uint32_t, DbChannelPtr> DbChannelMap;
typedef shared_ptr<DbChannelMap> DbChannelMapPtr;

/**
 * DeviceState:
 * - device_type_id: '1'
 *   id: '1'
 *   name: Out
 *   value: '1'
 */
class DbDeviceState : public DbEntry {
 public:  
  uint32_t deviceTypeId;
  uint32_t value;
  uint32_t mask;
  std::string name;
  
 DbDeviceState() : DbEntry(), deviceTypeId(0), value(0), mask(0), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceState * const devState) {
    os << "id[" << devState->id << "]; "
       << "deviceTypeId[" << devState->deviceTypeId << "]; "
       << "value[0x" << std::hex << devState->value << "]; "
       << "mask[0x" << devState->mask << std::dec << "]; "
       << "name[" << devState->name << "]";
    return os;
  }
};

typedef shared_ptr<DbDeviceState> DbDeviceStatePtr;
typedef std::map<uint32_t, DbDeviceStatePtr> DbDeviceStateMap;
typedef shared_ptr<DbDeviceStateMap> DbDeviceStateMapPtr;

/**
 * DeviceType:
 * - id: '1'
 *   name: Insertion Device
 */
class DbDeviceType : public DbEntry {
 public:
  std::string name;
  
  DbDeviceStateMapPtr deviceStates;

 DbDeviceType() : DbEntry(), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceType * const devType) {
    os << "id[" << devType->id << "]; "
       << "name[" << devType->name << "]; ";

    if (devType->deviceStates) {
      os << " deviceStates [";
      for (DbDeviceStateMap::iterator deviceState = devType->deviceStates->begin();
	   deviceState != devType->deviceStates->end(); ++deviceState) {
	os << (*deviceState).second->id << " ";
      }
      os << "]";
    }
    return os;
  }
};

typedef shared_ptr<DbDeviceType> DbDeviceTypePtr;
typedef std::map<uint32_t, DbDeviceTypePtr> DbDeviceTypeMap;
typedef shared_ptr<DbDeviceTypeMap> DbDeviceTypeMapPtr;

/**
 * DeviceInput:
 * - bit_position: '0'
 *   channel_id: '1'
 *   digital_device_id: '1'
 *   id: '1'
 */
class DbDeviceInput : public DbEntry {
 public:
  uint32_t bitPosition;
  uint32_t channelId;
  uint32_t faultValue;
  uint32_t digitalDeviceId;

  // Device input value must be read from the Central Node Firmware
  uint32_t value;

  // Latched value
  uint32_t latchedValue;

  // Pouint32_ter to the bypass for this input
  InputBypassPtr bypass;

  // Pointer to the Channel connected to the device
  DbChannelPtr channel;

 DbDeviceInput() : DbEntry(), 
    bitPosition(-1), channelId(-1), faultValue(0), digitalDeviceId(-1) {
  }

  // This should update the value from the data read from the central node firmware
  void update(uint32_t v) {
    value = v;

    // Latch new value if this is a fault
    if (v == faultValue) {
      latchedValue = faultValue;
    }
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceInput * const deviceInput) {
    os << "id[" << deviceInput->id << "]; " 
       << "bitPosition[" << deviceInput->bitPosition << "]; "
       << "faultValue[" << deviceInput->faultValue << "]; "
       << "channelId[" << deviceInput->channelId << "]; "
       << "digitalDeviceId[" << deviceInput->digitalDeviceId << "]; "
       << "value[" << deviceInput->value << "]";
    return os;
  }
};

typedef shared_ptr<DbDeviceInput> DbDeviceInputPtr;
typedef std::map<uint32_t, DbDeviceInputPtr> DbDeviceInputMap;
typedef shared_ptr<DbDeviceInputMap> DbDeviceInputMapPtr;


class DbFaultState;
typedef shared_ptr<DbFaultState> DbFaultStatePtr;
typedef std::map<uint32_t, DbFaultStatePtr> DbFaultStateMap;
typedef shared_ptr<DbFaultStateMap> DbFaultStateMapPtr;


/**
 * DigitalDevice:
 * - device_type_id: '1'
 *   id: '1'
 */
class DbDigitalDevice : public DbEntry {
 public:
  uint32_t deviceTypeId;
  uint32_t value; // calculated from the DeviceInputs for this device
  std::string name;
  std::string description;
  float zPosition;
  uint32_t evaluation;
  uint32_t cardId; // Application Card ID

  DbDeviceInputMapPtr inputDevices; // list built after the config is loaded

  DbDeviceTypePtr deviceType;

  // list of fault states - populated if the device evaluation happens in firmware
  // TODO: populate this map
  DbFaultStateMapPtr fastFaultStates; 
  uint16_t fastDestinationMask;
  uint16_t fastPowerClass;

 DbDigitalDevice() : DbEntry(), deviceTypeId(-1) {
  }

  void update(uint32_t v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbDigitalDevice * const digitalDevice) {
    os << "id[" << digitalDevice->id << "]; "
       << "deviceTypeId[" << digitalDevice->deviceTypeId << "]; "
       << "name[" << digitalDevice->name << "]; "
       << "z[" << digitalDevice->zPosition << "]; "
       << "evaluation[" << digitalDevice->evaluation << "]; "
       << "description[" << digitalDevice->description << "]; "
       << "cardId[" << digitalDevice->cardId << "]";
    return os;
  }
};

typedef shared_ptr<DbDigitalDevice> DbDigitalDevicePtr;
typedef std::map<uint32_t, DbDigitalDevicePtr> DbDigitalDeviceMap;
typedef shared_ptr<DbDigitalDeviceMap> DbDigitalDeviceMapPtr;


/**
 * AnalogDevice:
 * - analog_device_type_id: '1'
 *   channel_id: '1'
 *   id: '3'
 */
class DbAnalogDevice : public DbEntry {
 public:
  uint32_t deviceTypeId;
  uint32_t channelId;
  std::string name;
  std::string description;
  float zPosition;
  uint32_t evaluation;
  uint32_t cardId; // Application Card ID

  // Configured after loading the YAML file
  // Each bit represents a threshold state from the analog device
  // BPM devices: 3 bytes (X, Y, TMIT thresholds)
  // BLM devices: 4 bytes (one byte for each integration window)
  int32_t value; 

  // Latched value
  uint32_t latchedValue;

  DbDeviceTypePtr deviceType;

  // Pointer to the Channel connected to the device
  DbChannelPtr channel;

  // Pointer to the bypass for this input
  InputBypassPtr bypass;

  // list of fault states - populated if the device evaluation happens in firmware
  // TODO: populate this map
  DbFaultStateMapPtr fastFaultStates; 
  
 DbAnalogDevice() : DbEntry(), deviceTypeId(-1), channelId(-1), value(0), latchedValue(0) {
  }

  void update(uint32_t v) {
    //    std::cout << "Updating analog device [" << id << "] value=" << v << std::endl;
    value = v;

    // Latch new value if this there is a threshold at fault
    if (value | latchedValue != latchedValue) {
      latchedValue |= value;
    }
  }

  friend std::ostream & operator<<(std::ostream &os, DbAnalogDevice * const analogDevice) {
    os << "id[" << analogDevice->id << "]; "
       << "deviceTypeId[" << analogDevice->deviceTypeId << "]; "
       << "name[" << analogDevice->name << "]; "
       << "z[" << analogDevice->zPosition << "]; "
       << "description[" << analogDevice->description << "]; "
       << "evaluation[" << analogDevice->evaluation << "]; "
       << "channelId[" << analogDevice->channelId << "]; "
       << "cardId[" << analogDevice->cardId << "]";
    return os;
  }
};

typedef shared_ptr<DbAnalogDevice> DbAnalogDevicePtr;
typedef std::map<uint32_t, DbAnalogDevicePtr> DbAnalogDeviceMap;
typedef shared_ptr<DbAnalogDeviceMap> DbAnalogDeviceMapPtr;


/**
 * ApplicationCard:
 * - crate_id: '1'
 *   id: '1'
 *   number: '1'
 *   slot_number: '2'
 *   type_id: '1'
 *   global_id: 0
 *   name: "EIC Digital"
 *   description: "EIC Digital Status"
 */
class DbApplicationCard : public DbEntry {
 public:
  uint32_t number;
  uint32_t slotNumber;
  uint32_t crateId;
  uint32_t applicationTypeId;
  // Unique application ID, identifies the card were digital/analog signas are coming from
  uint32_t globalId;
  std::string name;
  std::string description;

  // Firmware configuration buffer
  //  ApplicationConfigBufferPtr applicationBuffer;
  ApplicationConfigBufferBitSet *applicationBuffer;
  //  shared_ptr<const DbCrate> crate;
  //  shared_ptr<const DbApplicationType> applicationType;

  // Each application has a list of analog or digital devices.
  // Only the devices that have 'fast' evaluation need
  // to be in this list - these maps are used to configure
  // the firmware logic
  DbAnalogDeviceMapPtr analogDevices;
  DbDigitalDeviceMapPtr digitalDevices;

 DbApplicationCard() : DbEntry(), 
  number(-1), slotNumber(-1), crateId(-1), applicationTypeId(-1) {
  }

  void writeConfiguration();
  void writeDigitalConfiguration();
  void writeAnalogConfiguration();

  friend std::ostream & operator<<(std::ostream &os, DbApplicationCard * const appCard) {
    os << "id[" << appCard->id << "]; " 
       << "number[" << appCard->number << "]; "
       << "crateId[" << appCard->crateId << "]; "
       << "slotNumber[" << appCard->slotNumber << "]; "
       << "applicationTypeId[" << appCard->applicationTypeId << "]; "
       << "globalId[" << appCard->globalId << "]; "
       << "name[" << appCard->name << "]; "
       << "description[" << appCard->description << "]; ";

    if (appCard->digitalDevices) {
      os << "DigitalDevices [";
      for (DbDigitalDeviceMap::iterator digitalDevice = appCard->digitalDevices->begin();
	   digitalDevice != appCard->digitalDevices->end(); ++digitalDevice) {
	os << (*digitalDevice).second->name << ", ";
      }
      os << "]";
    }
    else if (appCard->analogDevices) {
      os << "AnalogDevices [";
      for (DbAnalogDeviceMap::iterator analogDevice = appCard->analogDevices->begin();
	   analogDevice != appCard->analogDevices->end(); ++analogDevice) {
	os << (*analogDevice).second->name << ", ";
      }
      os << "]";
    }
    else {
      os << " - no devices (?)";
    }
    return os;
  }
};

typedef shared_ptr<DbApplicationCard> DbApplicationCardPtr;
typedef std::map<uint32_t, DbApplicationCardPtr> DbApplicationCardMap;
typedef shared_ptr<DbApplicationCardMap> DbApplicationCardMapPtr;

/**
 * FaultInput:
 * - bit_position: '0'
 *   device_id: '1'
 *   fault_id: '1'
 *   id: '1'
 */
class DbFaultInput : public DbEntry {
 public:
  // Values loaded from YAML file
  uint32_t faultId;
  uint32_t deviceId; // DbDigitalDevice or DbAnalogDevice
  uint32_t bitPosition;

  // Values calculated at run time
  uint32_t value; // Fault value calculated from the DeviceInputs
  // A fault input may come from a digital device or analog device threshold fault bits
  DbAnalogDevicePtr analogDevice;
  DbDigitalDevicePtr digitalDevice;
  
 DbFaultInput() : DbEntry(), faultId(-1), deviceId(-1), bitPosition(-1) {
  }

  // Update the value from the DeviceInputs
  void update() {
    value = 1;
  }

  friend std::ostream & operator<<(std::ostream &os, DbFaultInput * const crate) {
    os << "id[" << crate->id << "]; " 
       << "faultId[" << crate->faultId << "]; "
       << "deviceId[" << crate->deviceId << "]; "
       << "bitPosition[" << crate->bitPosition << "]";
    return os;
  }
};

typedef shared_ptr<DbFaultInput> DbFaultInputPtr;
typedef std::map<uint32_t, DbFaultInputPtr> DbFaultInputMap;
typedef shared_ptr<DbFaultInputMap> DbFaultInputMapPtr;


/**
 * BeamClass:
 * - id: '1'
 *   name: Class 1
 *   number: '1'
 */
class DbBeamClass : public DbEntry {
 public:
  uint32_t number;
  std::string name;
  std::string description;
  
 DbBeamClass() : DbEntry(), number(-1), name(""), description("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbBeamClass * const beamClass) {
    os << "id[" << beamClass->id << "]; "
       << "number[" << beamClass->number << "]; "
       << "name[" << beamClass->name << "] "
       << "description[" << beamClass->description << "]";
    return os;
  }
};

typedef shared_ptr<DbBeamClass> DbBeamClassPtr;
typedef std::map<uint32_t, DbBeamClassPtr> DbBeamClassMap;
typedef shared_ptr<DbBeamClassMap> DbBeamClassMapPtr;


/**
 * MitigationDevice:
 * - id: '1'
 *   name: Shutter
 *   destination_mask: 1
 */
class DbMitigationDevice : public DbEntry {
 public:
  std::string name;
  uint16_t destinationMask;

  // Used after loading the YAML file
  DbBeamClassPtr allowedBeamClass; // allowed beam class after evaluating faults
  DbBeamClassPtr tentativeBeamClass; // used while faults are being evaluated
  
 DbMitigationDevice() : DbEntry(), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbMitigationDevice * const mitigationDevice) {
    os << "id[" << mitigationDevice->id << "]; "
       << "name[" << mitigationDevice->name << "]; "
       << "destinationMask[" << mitigationDevice->destinationMask << "]";
    return os;
  }
};

typedef shared_ptr<DbMitigationDevice> DbMitigationDevicePtr;
typedef std::map<uint32_t, DbMitigationDevicePtr> DbMitigationDeviceMap;
typedef shared_ptr<DbMitigationDeviceMap> DbMitigationDeviceMapPtr;


/**
 * AllowedClass:
 * - beam_class_id: '1'
 *   fault_state_id: '1'
 *   id: '1'
 *   mitigation_device_id: '1'
 */
class DbAllowedClass : public DbEntry {
 public:
  uint32_t beamClassId;
  uint32_t faultStateId;
  uint32_t mitigationDeviceId;

  // Configured after loading the YAML file
  DbBeamClassPtr beamClass;
  DbMitigationDevicePtr mitigationDevice;
  
 DbAllowedClass() : DbEntry(), beamClassId(-1), faultStateId(-1), mitigationDeviceId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbAllowedClass * const allowedClass) {
    os << "id[" << allowedClass->id << "]; "
       << "beamClassId[" << allowedClass->beamClassId << "]; "
       << "faultStateId[" << allowedClass->faultStateId << "]; "
       << "mitigationDeviceId[" << allowedClass->mitigationDeviceId << "]";
    return os;
  }
};

typedef shared_ptr<DbAllowedClass> DbAllowedClassPtr;
typedef std::map<uint32_t, DbAllowedClassPtr> DbAllowedClassMap;
typedef shared_ptr<DbAllowedClassMap> DbAllowedClassMapPtr;


/**
 * FaultState:
 * - fault_id: '1'
 *   id: '1'
 *   name: Out
 *   value: '1'
 */
class DbFaultState : public DbEntry {
 public:
  uint32_t faultId;
  uint32_t deviceStateId;
  bool defaultState;
  //  uint32_t value;

  // Configured/Used after loading the YAML file
  bool faulted; // Evaluated based on the status of the deviceState
  bool ignored; // Fault state is ignored if there are valid ignore conditions 
  DbAllowedClassMapPtr allowedClasses; // Map of allowed classes (one for each mitigation device) for this fault states
  DbDeviceStatePtr deviceState;

 DbFaultState() : DbEntry(), faultId(0), deviceStateId(0),
    defaultState(false), faulted(true), ignored(false) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbFaultState * const digitalFault) {
    os << "id[" << digitalFault->id << "]; "
       << "faultId[" << digitalFault->faultId << "]; "
       << "deviceStateId[" << digitalFault->deviceStateId << "]; "
       << "faulted[" << digitalFault->faulted << "]; "
       << "default[" << digitalFault->defaultState << "]";
    if (digitalFault->deviceState) {
      os << ": DeviceState[name=" << digitalFault->deviceState->name
	 << ", value=" << digitalFault->deviceState->value << "]";
    }
    if (digitalFault->allowedClasses) {
      os << " : AllowedClasses[";
      unsigned int i = 1;
      for (DbAllowedClassMap::iterator it = digitalFault->allowedClasses->begin();
	   it != digitalFault->allowedClasses->end(); ++it, ++i) {
	os << (*it).second->mitigationDevice->name << "->"
	   << (*it).second->beamClass->name;
	if (i < digitalFault->allowedClasses->size()) {
	  os << ", ";
	}
      }
      os << "]";
    }

    return os;
  }
};


/** 
 * Fault: (these are digital faults types)
 *  - description: None
 *    id: '1'
 *    name: OTR Fault
 */
class DbFault : public DbEntry {
 public:
  std::string name;
  std::string description;

  // Configured after loading the YAML file
  bool faulted;
  bool ignored;
  DbFaultInputMapPtr faultInputs; // A fault may be built by several devices
  uint32_t value; // Calculated from the list of faultInputs
  DbFaultStateMapPtr faultStates; // Map of fault states for this fault
  DbFaultStatePtr defaultFaultState; // Default fault state if no other fault is active
                                                   // the default state not necessarily is a real fault

 DbFault() : DbEntry(), name(""), description(""), faulted(true), ignored(false) {
  }

  void update(uint32_t v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbFault * const fault) {
    os << "id[" << fault->id << "]; "
       << "name[" << fault->name << "]; "
       << "faulted[" << fault->faulted << "]; "
       << "ignored[" << fault->ignored << "]; "
       << "description[" << fault->description << "]";

    if (fault->faultInputs) {
      os << " : FaultInputs[";
      unsigned int i = 1;
      for (DbFaultInputMap::iterator it = fault->faultInputs->begin();
	   it != fault->faultInputs->end(); ++it, ++i) {
	if ((*it).second->digitalDevice) {
	  os << (*it).second->digitalDevice->name;
	  if (i < fault->faultInputs->size()) {
	    os << ", ";
	  }
	}
	else {
	  os << (*it).second->analogDevice->name;
	  if (i < fault->faultInputs->size()) {
	    os << ", ";
	  }
	}
      }
      os << "]";
    }

    if (fault->faultStates) {
      os << " : FaultStates[";
      unsigned int i = 1;
      for (DbFaultStateMap::iterator it = fault->faultStates->begin();
	   it != fault->faultStates->end(); ++it, ++i) {
	os << (*it).second->deviceState->name;
	if (i < fault->faultStates->size()) {
	  os << ", ";
	}
      }
      os << "]";
    }

    if (fault->defaultFaultState) {
      os << " : Defaul["
	 << fault->defaultFaultState->deviceState->name << "]";
    }

    return os;
  }
};

typedef shared_ptr<DbFault> DbFaultPtr;
typedef std::map<uint32_t, DbFaultPtr> DbFaultMap;
typedef shared_ptr<DbFaultMap> DbFaultMapPtr;

/**
 * ConditionInput:
 * - id: 1
 *   bit_position: 0
 *   fault_state_id: 1
 *   condition_id: 1
 */
class DbConditionInput : public DbEntry {
 public:
  // Values loaded from YAML file
  uint32_t bitPosition;
  uint32_t faultStateId;
  uint32_t conditionId;

  // The DbIgnoreCondition may point to a digital or analog fault state
  // TODO: this does not look good, these should be a single pointer, can't they?
  // This also happens with the DbIgnoreCondition class
  DbFaultStatePtr faultState;
  
 DbConditionInput() : DbEntry(), bitPosition(0), faultStateId(0), conditionId(0) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbConditionInput * const input) {
    os << "id[" << input->id << "]; " 
       << "bitPosition[" << input->bitPosition << "]; "
       << "faultStateId[" << input->faultStateId << "]; "
       << "conditionId[" << input->conditionId << "]";
    return os;
  }
};

typedef shared_ptr<DbConditionInput> DbConditionInputPtr;
typedef std::map<uint32_t, DbConditionInputPtr> DbConditionInputMap;
typedef shared_ptr<DbConditionInputMap> DbConditionInputMapPtr;

/**
 * IgnoreCondition:
 * - id: 1
 *   condition_id: 1
 *   fault_state_id: 14
 */
class DbIgnoreCondition : public DbEntry {
 public:
  uint32_t faultStateId;
  uint32_t conditionId;

  // The DbIgnoreCondition may point to a digital or analog fault state
  // TODO: this does not look good, these should be a single pointer, can't they?
  DbFaultStatePtr faultState;

 DbIgnoreCondition() : DbEntry(), faultStateId(0), conditionId(0) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbIgnoreCondition * const input) {
    os << "id[" << input->id << "]; " 
       << "faultStateId[" << input->faultStateId << "]; "
       << "conditionId[" << input->conditionId << "]";
    return os;
  }
};

typedef shared_ptr<DbIgnoreCondition> DbIgnoreConditionPtr;
typedef std::map<uint32_t, DbIgnoreConditionPtr> DbIgnoreConditionMap;
typedef shared_ptr<DbIgnoreConditionMap> DbIgnoreConditionMapPtr;

/** 
 * Condition:
 * - id: 1
 *   name: "YAG01_IN"
 *   description: "YAG01 screen IN"
 *   value: 1
 */
class DbCondition : public DbEntry {
 public:
  std::string name;
  std::string description;
  uint32_t mask;

  // Configured after loading the YAML file
  DbConditionInputMapPtr conditionInputs; // A condition is composed of one or more condition inputs
  bool state; // State is true when condition is met
  DbIgnoreConditionMapPtr ignoreConditions;

 DbCondition() : DbEntry(), name(""), description(""), mask(0) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbCondition * const fault) {
    os << "id[" << fault->id << "]; "
       << "name[" << fault->name << "]; "
       << "mask[0x" << fault->mask << std::dec << "]; "
       << "description[" << fault->description << "]";

    return os;
  }
};

typedef shared_ptr<DbCondition> DbConditionPtr;
typedef std::map<uint32_t, DbConditionPtr> DbConditionMap;
typedef shared_ptr<DbConditionMap> DbConditionMapPtr;


/**
 * Class containing all YAML MPS configuration
 */
class MpsDb {
 private:
  void setName(std::string yamlFileName);
  void configureAllowedClasses();
  void configureDeviceInputs();
  void configureDeviceTypes();
  void configureFaultInputs();
  void configureFaultStates();
  void configureAnalogDevices();
  void configureIgnoreConditions();
  void configureApplicationCards();

  /**
   * Memory that holds the firmware/hardware database configuration.
   * Every 0x100 chunk is reserved for an application configuration.
   * Even though there are 2048 bits on every chunk, the digital
   * configuration takes 1343 bits and the analog configuration takes
   * 1151 bits.
   */
  char fastConfigurationBuffer[NUM_APPLICATIONS * APPLICATION_CONFIG_BUFFER_SIZE];

 public:
  DbCrateMapPtr crates;
  DbApplicationTypeMapPtr applicationTypes;
  DbApplicationCardMapPtr applicationCards;
  DbChannelMapPtr digitalChannels;
  DbChannelMapPtr analogChannels;
  DbDeviceTypeMapPtr deviceTypes;
  DbDeviceStateMapPtr deviceStates;
  DbDigitalDeviceMapPtr digitalDevices;
  DbDeviceInputMapPtr deviceInputs;
  DbFaultMapPtr faults;
  DbFaultInputMapPtr faultInputs;
  DbFaultStateMapPtr faultStates;
  DbAnalogDeviceMapPtr analogDevices;
  DbMitigationDeviceMapPtr mitigationDevices;
  DbBeamClassMapPtr beamClasses;
  DbAllowedClassMapPtr allowedClasses;
  DbConditionMapPtr conditions;
  DbIgnoreConditionMapPtr ignoreConditions;
  DbConditionInputMapPtr conditionInputs;

  // Name of the loaded YAML file
  std::string name;

  // This is initialized by the configure() method, after loading the YAML file
  //  DbFaultStateMapPtr faultStates; 

  MpsDb();
  int load(std::string yamlFile);
  void configure();

  void showFaults();
  void showFault(DbFaultPtr fault);

  void writeFirmwareConfiguration();

  //  mpsDb->printMap<DbConditionMapPtr, DbConditionMap::iterator>(os, mpsDb->conditions, "Conditions");

  template<class MapPtrType, class IteratorType>
    void printMap(std::ostream &os, MapPtrType map,
	     std::string mapName) {
    os << mapName << ":" << std::endl;
    for (IteratorType it = map->begin(); 
	 it != map->end(); ++it) {
      os << "  " << (*it).second << std::endl;
    }
  }

  /**
   * Print contents of all entities
   */
  friend std::ostream & operator<<(std::ostream &os, MpsDb * const mpsDb) {
    os << "Name: " << mpsDb->name << std::endl;

    mpsDb->printMap<DbCrateMapPtr, DbCrateMap::iterator>
      (os, mpsDb->crates, "Crate");

    mpsDb->printMap<DbApplicationTypeMapPtr, DbApplicationTypeMap::iterator>
      (os, mpsDb->applicationTypes, "ApplicationType");

    mpsDb->printMap<DbApplicationCardMapPtr, DbApplicationCardMap::iterator>
      (os, mpsDb->applicationCards, "ApplicationCard");

    mpsDb->printMap<DbChannelMapPtr, DbChannelMap::iterator>
      (os, mpsDb->digitalChannels, "DigitalChannel");

    mpsDb->printMap<DbChannelMapPtr, DbChannelMap::iterator>
      (os, mpsDb->analogChannels, "AnalogChannel");

    mpsDb->printMap<DbDeviceTypeMapPtr, DbDeviceTypeMap::iterator>
      (os, mpsDb->deviceTypes, "DeviceType");

    mpsDb->printMap<DbDeviceStateMapPtr, DbDeviceStateMap::iterator>
      (os, mpsDb->deviceStates, "DeviceState");

    mpsDb->printMap<DbDigitalDeviceMapPtr, DbDigitalDeviceMap::iterator>
      (os, mpsDb->digitalDevices, "DigitalDevice");

    mpsDb->printMap<DbDeviceInputMapPtr, DbDeviceInputMap::iterator>
      (os, mpsDb->deviceInputs, "DeviceInput");

    mpsDb->printMap<DbFaultMapPtr, DbFaultMap::iterator>
      (os, mpsDb->faults, "Fault");

    mpsDb->printMap<DbFaultInputMapPtr, DbFaultInputMap::iterator>
      (os, mpsDb->faultInputs, "FaultInput");

    mpsDb->printMap<DbFaultStateMapPtr, DbFaultStateMap::iterator>
      (os, mpsDb->faultStates, "FaultState");

    mpsDb->printMap<DbAnalogDeviceMapPtr, DbAnalogDeviceMap::iterator>
      (os, mpsDb->analogDevices, "AnalogDevice");

    mpsDb->printMap<DbMitigationDeviceMapPtr, DbMitigationDeviceMap::iterator>
      (os, mpsDb->mitigationDevices, "MitigationDevice");

    mpsDb->printMap<DbBeamClassMapPtr, DbBeamClassMap::iterator>
      (os, mpsDb->beamClasses, "BeamClass");

    mpsDb->printMap<DbAllowedClassMapPtr, DbAllowedClassMap::iterator>
      (os, mpsDb->allowedClasses, "AllowedClass");

    mpsDb->printMap<DbConditionMapPtr, DbConditionMap::iterator>
      (os, mpsDb->conditions, "Conditions");

    mpsDb->printMap<DbConditionInputMapPtr, DbConditionInputMap::iterator>
      (os, mpsDb->conditionInputs, "ConditionInputs");

    mpsDb->printMap<DbIgnoreConditionMapPtr, DbIgnoreConditionMap::iterator>
      (os, mpsDb->ignoreConditions, "IgnoreConditions");

    return os;
  }
};

typedef shared_ptr<MpsDb> MpsDbPtr;

#endif
