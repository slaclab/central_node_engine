#ifndef CENTRAL_NODE_DATABASE_TABLES_H
#define CENTRAL_NODE_DATABASE_TABLES_H

#include <map>
#include <exception>
#include <iostream>
#include <bitset>
#include <central_node_database_defs.h>
#include <central_node_exception.h>
#include <central_node_bypass.h>
#include <central_node_history.h>
#include <stdint.h>
#include <time_util.h>

#include <pthread.h>
#include <boost/shared_ptr.hpp>

using boost::shared_ptr;
using boost::weak_ptr;

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

  DbEntry();
  friend std::ostream & operator<<(std::ostream &os, DbEntry * const entry);
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
  
  DbCrate();
  friend std::ostream & operator<<(std::ostream &os, DbCrate * const crate);
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

  DbApplicationType();
  friend std::ostream & operator<<(std::ostream &os, DbApplicationType * const appType);
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
  
  DbChannel();
  friend std::ostream & operator<<(std::ostream &os, DbChannel * const channel);
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
  
  DbDeviceState();
  friend std::ostream & operator<<(std::ostream &os, DbDeviceState * const devState);
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

  DbDeviceType();
  friend std::ostream & operator<<(std::ostream &os, DbDeviceType * const devType);
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
  uint32_t previousValue;

  // Latched value
  uint32_t latchedValue;

  // Count 'was high'=0 & 'was low'=0 
  uint32_t invalidValueCount;

  // Pouint32_ter to the bypass for this input
  InputBypassPtr bypass;

  // Pointer to the Channel connected to the device
  DbChannelPtr channel;

  // Set true if this input is used by a fast evaluated device (must be the only input to device)
  bool fastEvaluation;

  // Pointer to shared bitset buffer containing updates for all inputs from
  // the same application card
  ApplicationUpdateBufferBitSet *applicationUpdateBuffer;

  DbDeviceInput();

  void setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer);
  void update(uint32_t v);
  void update();

  friend std::ostream & operator<<(std::ostream &os, DbDeviceInput * const deviceInput);
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
  uint32_t evaluation;
  uint32_t cardId; // Application Card ID

  DbDeviceInputMapPtr inputDevices; // list built after the config is loaded

  DbDeviceTypePtr deviceType;

  /**
   * These fields get populated when the database is loaded, they are
   * used to configure the central node firmware with fast fault
   * information.
   */
  // 16-bit beam destination mask for fast digital devices/faults
  uint16_t fastDestinationMask;

  // 4-bit encoded max beam power class for faults from this device
  uint16_t fastPowerClass;

  // This is the value that must be present when there is no fault,
  // when the digital device has a value different from the expected
  // the firmware will apply the fastPowerClass/fastDestinationMask
  uint8_t fastExpectedState;

  DbDigitalDevice();

  void update(uint32_t v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbDigitalDevice * const digitalDevice);
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
  uint32_t evaluation;
  uint32_t cardId; // Application Card ID

  // Configured after loading the YAML file
  // Each bit represents a threshold state from the analog device
  // BPM devices: 3 bytes (X, Y, TMIT thresholds)
  // BLM devices: 4 bytes (one byte for each integration window)
  int32_t value; 
  int32_t previousValue;

  // Latched value
  uint32_t latchedValue;

  // Count 'was high'=0 & 'was low'=0 
  uint32_t invalidValueCount;

  DbDeviceTypePtr deviceType;

  // Pointer to the Channel connected to the device
  DbChannelPtr channel;

  // Array of pointer to bypasses, one for each threshold (max of 32 thresholds).
  // The bypasses are managed by the BypassManager class, when a bypass is added or
  // expires the bypassMask for the AnalogDevice is updated.
  //
  // TODO: bypass is possible in HW only per integrator, not by threshold!
  //  InputBypassPtr bypass[ANALOG_DEVICE_NUM_THRESHOLDS]; 
  InputBypassPtr bypass[ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL];

  // Bypass mask composed from all active threshold bypasses. This value gets updated
  // whenever a threshold bypass becomes valid or expires. The BypassManager changes this 
  // mask directly. The Engine always does a bitwise AND between this mask and the value read
  // from firmware. If a threshold is bypassed the bypassMask is 0 at the threshold position,
  // and 1 otherwise.
  uint32_t bypassMask; 

  /**
   * These fields get populated when the database is loaded, they are
   * used to configure the central node firmware with fast fault
   * information.
   */
  // 16-bit beam destination mask for fast digital devices/faults
  // there is a destination mask per integrator. 
  uint16_t fastDestinationMask[ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL];

  // 4-bit encoded max beam power class for each integrator threshold
  uint16_t fastPowerClass[ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL * ANALOG_CHANNEL_INTEGRATORS_SIZE];

  // Pointer to shared bitset buffer containing updates for all inputs from
  // the same application card
  ApplicationUpdateBufferBitSet *applicationUpdateBuffer;
  
  DbAnalogDevice();

  void update(uint32_t v);
  void update();

  void setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer);

  friend std::ostream & operator<<(std::ostream &os, DbAnalogDevice * const analogDevice);
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
  bool online; // True if received non-zero update last 360Hz update period

  // Memory containing firmware configuration buffer
  ApplicationConfigBufferBitSet *applicationConfigBuffer;

  // Memory containing input updates from firmware
  ApplicationUpdateBufferBitSet *applicationUpdateBuffer;

  // Each application has a list of analog or digital devices.
  // Only the devices that have 'fast' evaluation need
  // to be in this list - these maps are used to configure
  // the firmware logic
  DbAnalogDeviceMapPtr analogDevices;
  DbDigitalDeviceMapPtr digitalDevices;

  DbApplicationCard();

  void writeConfiguration();
  void writeDigitalConfiguration();
  void writeAnalogConfiguration();

  void printAnalogConfiguration();

  void configureUpdateBuffers();
  void updateInputs();
  bool isAnalog();
  bool isDigital();

  friend std::ostream & operator<<(std::ostream &os, DbApplicationCard * const appCard);
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
  
  DbFaultInput();

  // Update the value from the DeviceInputs
  void update() {
    value = 1;
  }

  friend std::ostream & operator<<(std::ostream &os, DbFaultInput * const crate);
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
  uint32_t integrationWindow;
  uint32_t minPeriod;
  uint32_t totalCharge;
  
  DbBeamClass();
  friend std::ostream & operator<<(std::ostream &os, DbBeamClass * const beamClass);
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
  DbBeamClassPtr previousAllowedBeamClass; // beam class set previously
  DbBeamClassPtr allowedBeamClass; // allowed beam class after evaluating faults
  DbBeamClassPtr tentativeBeamClass; // used while faults are being evaluated
  
  // Memory location where the allowed beam class for this device
  // is written and sent to firmware
  uint32_t *softwareMitigationBuffer;
  uint8_t softwareMitigationBufferIndex;
  uint8_t bitShift; // define if mitigation uses high/low 4-bits in the softwareMitigationBuffer

  DbMitigationDevice();

  void setAllowedBeamClass() {
    allowedBeamClass = tentativeBeamClass;
    softwareMitigationBuffer[softwareMitigationBufferIndex] |= ((allowedBeamClass->number & 0xF) << bitShift);

    if (previousAllowedBeamClass->number != allowedBeamClass->number) {
      History::getInstance().logMitigation(id, previousAllowedBeamClass->id, allowedBeamClass->id);
    }

  }

  friend std::ostream & operator<<(std::ostream &os, DbMitigationDevice * const mitigationDevice);
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
  
  DbAllowedClass();

  friend std::ostream & operator<<(std::ostream &os, DbAllowedClass * const allowedClass);
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

  DbFaultState();
  friend std::ostream & operator<<(std::ostream &os, DbFaultState * const digitalFault);
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
  bool faultLatched;
  bool ignored;
  uint32_t evaluation; // Set according to the input device types
  DbFaultInputMapPtr faultInputs; // A fault may be built by several devices
  uint32_t value; // Calculated from the list of faultInputs
  DbFaultStateMapPtr faultStates; // Map of fault states for this fault
  DbFaultStatePtr defaultFaultState; // Default fault state if no other fault is active
                                                   // the default state not necessarily is a real fault

  DbFault();

  void update(uint32_t v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbFault * const fault);
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
  
  DbConditionInput();
  friend std::ostream & operator<<(std::ostream &os, DbConditionInput * const input);
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

  DbIgnoreCondition();
  friend std::ostream & operator<<(std::ostream &os, DbIgnoreCondition * const input);
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

  DbCondition();
  friend std::ostream & operator<<(std::ostream &os, DbCondition * const fault);
};

typedef shared_ptr<DbCondition> DbConditionPtr;
typedef std::map<uint32_t, DbConditionPtr> DbConditionMap;
typedef shared_ptr<DbConditionMap> DbConditionMapPtr;

#endif
