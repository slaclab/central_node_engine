#ifndef CENTRAL_NODE_DATABASE_H
#define CENTRAL_NODE_DATABASE_H

#include <map>
#include <exception>
#include <iostream>
#include <central_node_exception.h>
#include <central_node_bypass.h>

#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

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
  int id;

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
  int number;
  int numSlots;
  int shelfNumber;
  
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
typedef std::map<int, DbCratePtr> DbCrateMap;
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
  int number;
  int analogChannelCount;
  int analogChannelSize;
  int digitalChannelCount;
  int digitalChannelSize;
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
typedef std::map<int, DbApplicationTypePtr> DbApplicationTypeMap;
typedef shared_ptr<DbApplicationTypeMap> DbApplicationTypeMapPtr;


/**
 * ApplicationCard:
 * - crate_id: '1'
 *   id: '1'
 *   number: '1'
 *   slot_number: '2'
 *   type_id: '1'
 */
class DbApplicationCard : public DbEntry {
 public:
  int number;
  int slotNumber;
  int crateId;
  int applicationTypeId;
  //  shared_ptr<const DbCrate> crate;
  //  shared_ptr<const DbApplicationType> applicationType;

 DbApplicationCard() : DbEntry(), 
  number(-1), slotNumber(-1), crateId(-1), applicationTypeId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbApplicationCard * const appCard) {
    os << "id[" << appCard->id << "]; " 
       << "number[" << appCard->number << "]; "
       << "crateId[" << appCard->crateId << "]; "
       << "slotNumber[" << appCard->slotNumber << "]; "
       << "applicationTypeId[" << appCard->applicationTypeId << "];";
    return os;
  }
};

typedef shared_ptr<DbApplicationCard> DbApplicationCardPtr;
typedef std::map<int, DbApplicationCardPtr> DbApplicationCardMap;
typedef shared_ptr<DbApplicationCardMap> DbApplicationCardMapPtr;

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
  int number;
  int cardId;
  
 DbChannel() : DbEntry(), number(-1), cardId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbChannel * const channel) {
    os << "id[" << channel->id << "]; " 
       << "number[" << channel->number << "]; "
       << "cardId[" << channel->cardId << "];";
    return os;
  }
};

typedef shared_ptr<DbChannel> DbChannelPtr;
typedef std::map<int, DbChannelPtr> DbChannelMap;
typedef shared_ptr<DbChannelMap> DbChannelMapPtr;

/**
 * DeviceType:
 * - id: '1'
 *   name: Insertion Device
 */
class DbDeviceType : public DbEntry {
 public:
  std::string name;
  
 DbDeviceType() : DbEntry(), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceType * const devType) {
    os << "id[" << devType->id << "]; "
       << "name[" << devType->name << "];";
    return os;
  }
};

typedef shared_ptr<DbDeviceType> DbDeviceTypePtr;
typedef std::map<int, DbDeviceTypePtr> DbDeviceTypeMap;
typedef shared_ptr<DbDeviceTypeMap> DbDeviceTypeMapPtr;

/**
 * DeviceState:
 * - device_type_id: '1'
 *   id: '1'
 *   name: Out
 *   value: '1'
 */
class DbDeviceState : public DbEntry {
 public:  
  int deviceTypeId;
  int value;
  std::string name;
  
 DbDeviceState() : DbEntry(), deviceTypeId(-1), value(-1), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceState * const devState) {
    os << "id[" << devState->id << "]; "
       << "deviceTypeId[" << devState->deviceTypeId << "]; "
       << "value[" << devState->value << "]; "
       << "name[" << devState->name << "]";
    return os;
  }
};

typedef shared_ptr<DbDeviceState> DbDeviceStatePtr;
typedef std::map<int, DbDeviceStatePtr> DbDeviceStateMap;
typedef shared_ptr<DbDeviceStateMap> DbDeviceStateMapPtr;

/**
 * DeviceInput:
 * - bit_position: '0'
 *   channel_id: '1'
 *   digital_device_id: '1'
 *   id: '1'
 */
class DbDeviceInput : public DbEntry {
 public:
  int bitPosition;
  int channelId;
  int digitalDeviceId;

  // Device input value must be read from the Central Node Firmware
  int value;

  // Pointer to the bypass for this input
  InputBypassPtr bypass;

  // Pointer to the Channel connected to the device
  DbChannelPtr channel;

 DbDeviceInput() : DbEntry(), 
    bitPosition(-1), channelId(-1), digitalDeviceId(-1) {
  }

  // This should update the value from the data read from the central node firmware
  void update(int v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceInput * const deviceInput) {
    os << "id[" << deviceInput->id << "]; " 
       << "bitPosition[" << deviceInput->bitPosition << "]; "
       << "channelId[" << deviceInput->channelId << "]; "
       << "digitalDeviceId[" << deviceInput->digitalDeviceId << "]; "
       << "value[" << deviceInput->value << "]";
    return os;
  }
};

typedef shared_ptr<DbDeviceInput> DbDeviceInputPtr;
typedef std::map<int, DbDeviceInputPtr> DbDeviceInputMap;
typedef shared_ptr<DbDeviceInputMap> DbDeviceInputMapPtr;

/**
 * DigitalDevice:
 * - device_type_id: '1'
 *   id: '1'
 */
class DbDigitalDevice : public DbEntry {
 public:
  int deviceTypeId;
  int value; // calculated from the DeviceInputs for this device
  DbDeviceInputMapPtr inputDevices; // list built after the config is loaded

 DbDigitalDevice() : DbEntry(), deviceTypeId(-1) {
  }

  void update(int v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbDigitalDevice * const digitalDevice) {
    os << "id[" << digitalDevice->id << "]; "
       << "deviceTypeId[" << digitalDevice->deviceTypeId << "];";
    return os;
  }
};

typedef shared_ptr<DbDigitalDevice> DbDigitalDevicePtr;
typedef std::map<int, DbDigitalDevicePtr> DbDigitalDeviceMap;
typedef shared_ptr<DbDigitalDeviceMap> DbDigitalDeviceMapPtr;

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
  int faultId;
  int deviceId; // DbDigitalDevice
  int bitPosition;

  // Values calculated at run time
  int value; // Fault value calculated from the DeviceInputs
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
typedef std::map<int, DbFaultInputPtr> DbFaultInputMap;
typedef shared_ptr<DbFaultInputMap> DbFaultInputMapPtr;


/**
 * ThresholdValue:
 * - id: '1'
 *   threshold: '0'
 *   threshold_value_map_id: '1'
 *   value: '0.0'
 */
class DbThresholdValue : public DbEntry {
 public:
  int threshold;
  int thresholdValueMapId;
  float value;
  
 DbThresholdValue() : DbEntry(), threshold(-1), thresholdValueMapId(-1), value(0.0) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbThresholdValue * const thresValue) {
    os << "id[" << thresValue->id << "]; "
       << "threshold[" << thresValue->threshold << "]; "
       << "thresholdValueMapId[" << thresValue->thresholdValueMapId << "]; "
       << "value[" << thresValue->value << "]";

    return os;
  }
};

typedef shared_ptr<DbThresholdValue> DbThresholdValuePtr;
typedef std::map<int, DbThresholdValuePtr> DbThresholdValueMap;
typedef shared_ptr<DbThresholdValueMap> DbThresholdValueMapPtr;

/**
 * ThresholdValueMap:
 * - description: Map for generic PICs.
 *   id: '1'
 */
class DbThresholdValueMapEntry : public DbEntry {
 public:
  std::string description;

  // Configured after loading the YAML file
  DbThresholdValueMapPtr thresholdValues; // List of all thresholds for this ThresholdValueMap

 DbThresholdValueMapEntry() : DbEntry(), description("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbThresholdValueMapEntry * const thresValueMapEntry) {
    os << "id[" << thresValueMapEntry->id << "]; "
       << "description[" << thresValueMapEntry->description << "]";
    return os;
  }
};

typedef shared_ptr<DbThresholdValueMapEntry> DbThresholdValueMapEntryPtr;
typedef std::map<int, DbThresholdValueMapEntryPtr> DbThresholdValueMapEntryMap;
typedef shared_ptr<DbThresholdValueMapEntryMap> DbThresholdValueMapEntryMapPtr;


/**
 * AnalogDeviceType:
 * - id: '1'
 *   name: PIC
 *   threshold_value_map_id: '1'
 *   units: counts
 */
class DbAnalogDeviceType : public DbEntry {
 public:
  std::string name;
  int thresholdValueMapId;
  std::string units;

  // Configured after loading the YAML file
  DbThresholdValueMapEntryPtr thresholdMapEntry; // Has a list of thresholds for this device
  
 DbAnalogDeviceType() : DbEntry(), name(""), thresholdValueMapId(-1), units("") {
  }

  friend std::ostream & operator<<(std::ostream &os,
				   DbAnalogDeviceType * const analogDeviceType) {
    os << "id[" << analogDeviceType->id << "]; "
       << "thresholdValueMapId[" << analogDeviceType->thresholdValueMapId << "]; "
       << "name[" << analogDeviceType->name << "]; "
       << "units[" << analogDeviceType->units << "]";
    return os;
  }
};

typedef shared_ptr<DbAnalogDeviceType> DbAnalogDeviceTypePtr;
typedef std::map<int, DbAnalogDeviceTypePtr> DbAnalogDeviceTypeMap;
typedef shared_ptr<DbAnalogDeviceTypeMap> DbAnalogDeviceTypeMapPtr;

/**
 * AnalogDevice:
 * - analog_device_type_id: '1'
 *   channel_id: '1'
 *   id: '3'
 */
class DbAnalogDevice : public DbEntry {
 public:
  int analogDeviceTypeId;
  int channelId;
  
  // Configured after loading the YAML file
  float value; // Analog value from read from the Central Node Firmware
  DbAnalogDeviceTypePtr analogDeviceType;

  // Pointer to the Channel connected to the device
  DbChannelPtr channel;

  // Pointer to the bypass for this input
  InputBypassPtr bypass;
  
 DbAnalogDevice() : DbEntry(), analogDeviceTypeId(-1), channelId(-1), value(0) {
  }

  void update(float v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbAnalogDevice * const analogDevice) {
    os << "id[" << analogDevice->id << "]; "
       << "analogDeviceTypeId[" << analogDevice->analogDeviceTypeId << "]; "
       << "channelId[" << analogDevice->channelId << "]";
    return os;
  }
};

typedef shared_ptr<DbAnalogDevice> DbAnalogDevicePtr;
typedef std::map<int, DbAnalogDevicePtr> DbAnalogDeviceMap;
typedef shared_ptr<DbAnalogDeviceMap> DbAnalogDeviceMapPtr;

/**
 * BeamClass:
 * - id: '1'
 *   name: Class 1
 *   number: '1'
 */
class DbBeamClass : public DbEntry {
 public:
  int number;
  std::string name;
  
 DbBeamClass() : DbEntry(), number(-1), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbBeamClass * const beamClass) {
    os << "id[" << beamClass->id << "]; "
       << "number[" << beamClass->number << "]; "
       << "name[" << beamClass->name << "]";
    return os;
  }
};

typedef shared_ptr<DbBeamClass> DbBeamClassPtr;
typedef std::map<int, DbBeamClassPtr> DbBeamClassMap;
typedef shared_ptr<DbBeamClassMap> DbBeamClassMapPtr;


/**
 * MitigationDevice:
 * - id: '1'
 *   name: Gun
 */
class DbMitigationDevice : public DbEntry {
 public:
  std::string name;

  // Used after loading the YAML file
  DbBeamClassPtr allowedBeamClass; // allowed beam class after evaluating faults
  DbBeamClassPtr tentativeBeamClass; // used while faults are being evaluated
  
 DbMitigationDevice() : DbEntry(), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbMitigationDevice * const mitigationDevice) {
    os << "id[" << mitigationDevice->id << "]; "
       << "name[" << mitigationDevice->name << "]";
    return os;
  }
};

typedef shared_ptr<DbMitigationDevice> DbMitigationDevicePtr;
typedef std::map<int, DbMitigationDevicePtr> DbMitigationDeviceMap;
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
  int beamClassId;
  int faultStateId; // This can be a DigitalFaultState or a ThresholdFaultState
  int mitigationDeviceId;

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
typedef std::map<int, DbAllowedClassPtr> DbAllowedClassMap;
typedef shared_ptr<DbAllowedClassMap> DbAllowedClassMapPtr;

/**
 * ThresholdFault:
 * - analog_device_id: '3'
 *   greater_than: 'True'
 *   id: '1'
 *   name: PIC Loss > 1.0
 *   threshold: '1.0'
 */
class DbThresholdFault : public DbEntry {
 public:
  std::string name;
  float threshold;
  bool greaterThan;
  int analogDeviceId;
  
  // Configured after loading the YAML file
  DbAnalogDevicePtr analogDevice; // Device that can generate this fault
  float value; // Analog value from the device
  //  DbThresholdFaultStateMapPtr thresholdFaultStates; // Map of threshold fault states for this fault

 DbThresholdFault() : DbEntry(), name(""), threshold(0.0),
    greaterThan(true), analogDeviceId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os,
				   DbThresholdFault * const thresFault) {
    os << "id[" << thresFault->id << "]; "
       << "threshold[" << thresFault->threshold << "]; "
       << "greaterThan[" << thresFault->greaterThan << "]; "
       << "analogDeviceId[" << thresFault->analogDeviceId << "]; "
       << "name[" << thresFault->name << "]";
    return os;
  }
};

typedef shared_ptr<DbThresholdFault> DbThresholdFaultPtr;
typedef std::map<int, DbThresholdFaultPtr> DbThresholdFaultMap;
typedef shared_ptr<DbThresholdFaultMap> DbThresholdFaultMapPtr;

/**
 * ThresholdFaultState:
 * - id: '8'
 *   threshold_fault_id: '1'
 */
class DbThresholdFaultState : public DbEntry {
 public:
  int thresholdFaultId;
  
  // Configured/Used after loading the YAML file
  bool faulted;
  DbAllowedClassMapPtr allowedClasses; // Map of allowed classes (one for each
                                       // mitigation device) for this fault state
  DbThresholdFaultPtr thresholdFault;

 DbThresholdFaultState() : DbEntry(), thresholdFaultId(-1), faulted(false) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbThresholdFaultState * const thresFaultState) {
    os << "id[" << thresFaultState->id << "]; "
       << "thresholdFaultId[" << thresFaultState->thresholdFaultId << "]";
    return os;
  }
};

typedef shared_ptr<DbThresholdFaultState> DbThresholdFaultStatePtr;
typedef std::map<int, DbThresholdFaultStatePtr> DbThresholdFaultStateMap;
typedef shared_ptr<DbThresholdFaultStateMap> DbThresholdFaultStateMapPtr;

/**
 * DigitalFaultState:
 * - fault_id: '1'
 *   id: '1'
 *   name: Out
 *   value: '1'
 */
class DbDigitalFaultState : public DbEntry {
 public:
  int faultId;
  int value;
  std::string name;

  // Configured/Used after loading the YAML file
  bool faulted;
  DbAllowedClassMapPtr allowedClasses; // Map of allowed classes (one for each mitigation device) for this fault states

 DbDigitalFaultState() : DbEntry(), faultId(-1), value(-1), name(""), faulted(false) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDigitalFaultState * const digitalFault) {
    os << "id[" << digitalFault->id << "]; "
       << "faultId[" << digitalFault->faultId << "]; "
       << "value[" << digitalFault->value << "]; "
       << "name[" << digitalFault->name << "]";
    return os;
  }
};

typedef shared_ptr<DbDigitalFaultState> DbDigitalFaultStatePtr;
typedef std::map<int, DbDigitalFaultStatePtr> DbDigitalFaultStateMap;
typedef shared_ptr<DbDigitalFaultStateMap> DbDigitalFaultStateMapPtr;


/** 
 * Fault: (these are digital faults)
 *  - description: None
 *    id: '1'
 *    name: OTR Fault
 */
class DbFault : public DbEntry {
 public:
  std::string name;
  std::string description;

  // Configured after loading the YAML file
  DbFaultInputMapPtr faultInputs; // A fault may be built by several devices
  int value; // Calculated from the list of faultInputs
  DbDigitalFaultStateMapPtr digitalFaultStates; // Map of fault states for this fault

 DbFault() : DbEntry(), name(""), description("") {
  }

  void update(int v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbFault * const fault) {
    os << "id[" << fault->id << "]; "
       << "name[" << fault->name << "]; "
       << "description[" << fault->description << "]";
    return os;
  }
};

typedef shared_ptr<DbFault> DbFaultPtr;
typedef std::map<int, DbFaultPtr> DbFaultMap;
typedef shared_ptr<DbFaultMap> DbFaultMapPtr;


/**
 * Class containing all YAML MPS configuration
 */
class MpsDb {
 private:
  void setName(std::string yamlFileName);
  void configureAllowedClasses();
  void configureDeviceInputs();
  void configureFaultInputs();
  void configureThresholdFaults();
  void configureDigitalFaultStates();
  void configureThresholdValues();
  void configureAnalogDeviceTypes();
  void configureAnalogDevices();
  void configureThresholdFaultStates();

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
  DbDigitalFaultStateMapPtr digitalFaultStates;
  DbThresholdValueMapEntryMapPtr thresholdValueMaps;
  DbThresholdValueMapPtr thresholdValues; // All thresholds for all types
  DbAnalogDeviceTypeMapPtr analogDeviceTypes;
  DbAnalogDeviceMapPtr analogDevices;
  DbThresholdFaultMapPtr thresholdFaults;
  DbThresholdFaultStateMapPtr thresholdFaultStates;
  DbMitigationDeviceMapPtr mitigationDevices;
  DbBeamClassMapPtr beamClasses;
  DbAllowedClassMapPtr allowedClasses;

  // Name of the loaded YAML file
  std::string name;

  // This is initialized by the configure() method, after loading the YAML file
  //  DbFaultStateMapPtr faultStates; 

  int load(std::string yamlFile);
  void configure();

  void showFaults();
  void showFault(DbFaultPtr fault);
  void showThresholdFault(DbThresholdFaultPtr fault);

  /**
   * Print contents of all entities
   */
  friend std::ostream & operator<<(std::ostream &os, MpsDb * const mpsDb) {
    os << "Name: " << mpsDb->name << std::endl;
    os << "Crates: " << std::endl;
    for (DbCrateMap::iterator it = mpsDb->crates->begin(); 
	 it != mpsDb->crates->end(); ++it) {
      os << "  " << (*it).second << std::endl;
    }

    os << "ApplicationTypes: " << std::endl;
    for (DbApplicationTypeMap::iterator it = mpsDb->applicationTypes->begin(); 
	 it != mpsDb->applicationTypes->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "ApplicationCards: " << std::endl;
    for (DbApplicationCardMap::iterator it = mpsDb->applicationCards->begin(); 
	 it != mpsDb->applicationCards->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "DigitalChannels: " << std::endl;
    for (DbChannelMap::iterator it = mpsDb->digitalChannels->begin(); 
	 it != mpsDb->digitalChannels->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "AnalogChannels: " << std::endl;
    for (DbChannelMap::iterator it = mpsDb->analogChannels->begin(); 
	 it != mpsDb->analogChannels->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "DeviceTypes: " << std::endl;
    for (DbDeviceTypeMap::iterator it = mpsDb->deviceTypes->begin(); 
	 it != mpsDb->deviceTypes->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "DeviceStates: " << std::endl;
    for (DbDeviceStateMap::iterator it = mpsDb->deviceStates->begin(); 
	 it != mpsDb->deviceStates->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "DigitalDevices: " << std::endl;
    for (DbDigitalDeviceMap::iterator it = mpsDb->digitalDevices->begin(); 
	 it != mpsDb->digitalDevices->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "DeviceInputs: " << std::endl;
    for (DbDeviceInputMap::iterator it = mpsDb->deviceInputs->begin(); 
	 it != mpsDb->deviceInputs->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "Faults: " << std::endl;
    for (DbFaultMap::iterator it = mpsDb->faults->begin(); 
	 it != mpsDb->faults->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "FaultInputs: " << std::endl;
    for (DbFaultInputMap::iterator it = mpsDb->faultInputs->begin(); 
	 it != mpsDb->faultInputs->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "DigitalFaultStates: " << std::endl;
    for (DbDigitalFaultStateMap::iterator it = mpsDb->digitalFaultStates->begin(); 
	 it != mpsDb->digitalFaultStates->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "ThresholdValueMaps: " << std::endl;
    for (DbThresholdValueMapEntryMap::iterator it = mpsDb->thresholdValueMaps->begin(); 
	 it != mpsDb->thresholdValueMaps->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "AnalogDeviceTypes: " << std::endl;
    for (DbAnalogDeviceTypeMap::iterator it = mpsDb->analogDeviceTypes->begin(); 
	 it != mpsDb->analogDeviceTypes->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "AnalogDevices: " << std::endl;
    for (DbAnalogDeviceMap::iterator it = mpsDb->analogDevices->begin(); 
	 it != mpsDb->analogDevices->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "ThresholdValues: " << std::endl;
    for (DbThresholdValueMap::iterator it = mpsDb->thresholdValues->begin(); 
	 it != mpsDb->thresholdValues->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "ThresholdFaults: " << std::endl;
    for (DbThresholdFaultMap::iterator it = mpsDb->thresholdFaults->begin(); 
	 it != mpsDb->thresholdFaults->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "ThresholdFaultStates: " << std::endl;
    for (DbThresholdFaultStateMap::iterator it = mpsDb->thresholdFaultStates->begin(); 
	 it != mpsDb->thresholdFaultStates->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "MitigationDevices: " << std::endl;
    for (DbMitigationDeviceMap::iterator it = mpsDb->mitigationDevices->begin(); 
	 it != mpsDb->mitigationDevices->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "BeamClasss: " << std::endl;
    for (DbBeamClassMap::iterator it = mpsDb->beamClasses->begin(); 
	 it != mpsDb->beamClasses->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }

    os << "AllowedClasss: " << std::endl;
    for (DbAllowedClassMap::iterator it = mpsDb->allowedClasses->begin(); 
	 it != mpsDb->allowedClasses->end(); ++it) {
      os << "  " <<  (*it).second << std::endl;
    }


    return os;
  }
};

typedef shared_ptr<MpsDb> MpsDbPtr;

#endif
