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
 * DbCrate YAML class
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
 * DbInfo YAML class
 */
class DbInfo {
 public:
  std::string source;
  std::string date;
  std::string user;
  std::string md5sum;

  DbInfo();
  friend std::ostream & operator<<(std::ostream &os, DbInfo * const dbInfo);
};

typedef shared_ptr<DbInfo> DbInfoPtr;
typedef std::map<uint32_t, DbInfoPtr> DbInfoMap;
typedef shared_ptr<DbInfoMap> DbInfoMapPtr;

/**
 * DbApplicationType YAML class
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
 * DbChannel class for DbDigitalChannel and DbAnalogChannel
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
 * DbDeviceState YAML class
 */
class DbDeviceState : public DbEntry {
 public:
  uint32_t deviceTypeId;
  uint32_t value;
  uint32_t mask;
  std::string name;

  DbDeviceState();
  int getIntegrator();
  friend std::ostream & operator<<(std::ostream &os, DbDeviceState * const devState);
};

typedef shared_ptr<DbDeviceState> DbDeviceStatePtr;
typedef std::map<uint32_t, DbDeviceStatePtr> DbDeviceStateMap;
typedef shared_ptr<DbDeviceStateMap> DbDeviceStateMapPtr;

class DbApplicationCardInput {
 public:
  ApplicationUpdateBufferBitSetHalf *wasLowBuffer;
  ApplicationUpdateBufferBitSetHalf *wasHighBuffer;

  void setUpdateBuffers(ApplicationUpdateBufferBitSetHalf *wasLowBuf,
			ApplicationUpdateBufferBitSetHalf *wasHighBuf);

  uint32_t getWasLow(int channel);
  uint32_t getWasHigh(int channel);
};

/**
 * DbDeviceType YAML class
 */
class DbDeviceType : public DbEntry {
 public:
  std::string name;
  uint32_t numIntegrators; // number of 8-bit threshold comparators coming from device

  DbDeviceStateMapPtr deviceStates;

  DbDeviceType();
  friend std::ostream & operator<<(std::ostream &os, DbDeviceType * const devType);
};

typedef shared_ptr<DbDeviceType> DbDeviceTypePtr;
typedef std::map<uint32_t, DbDeviceTypePtr> DbDeviceTypeMap;
typedef shared_ptr<DbDeviceTypeMap> DbDeviceTypeMapPtr;

/**
 * DbDeviceInput YAML class
 */
class DbDeviceInput : public DbEntry, public DbApplicationCardInput {
 public:
  uint32_t bitPosition;
  uint32_t channelId;
  uint32_t faultValue;
  uint32_t digitalDeviceId;

  // Device input value must be read from the Central Node Firmware
  uint32_t wasLowBit;
  uint32_t wasHighBit;
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

  DbDeviceInput();

  //  void setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer);

  void unlatch();
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
 * DbDigitalDevice YAML class
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
 * DbAnalogDevice YAML class
 */
class DbAnalogDevice : public DbEntry, public DbApplicationCardInput {
 public:
  uint32_t deviceTypeId;
  uint32_t channelId;
  std::string name;
  std::string description;
  uint32_t evaluation;
  uint32_t cardId; // Application Card ID

  // Configured after loading the YAML file
  // Each bit represents a threshold state from the analog device
  // Analog devices: 1 byte (only one integration window)
  // BPM devices: 3 bytes (X, Y, TMIT thresholds)
  // BLM devices: 4 bytes (one byte for each integration window)
  int32_t value;
  int32_t previousValue;

  // Latched value
  uint32_t latchedValue;

  // Count 'was high'=0 & 'was low'=0
  uint32_t invalidValueCount;

  // Faults from this devices are bypassed when ignored==true
  bool ignored;

  // Faults from the integrators from this device are bypassed when ignored[integrator]==true
  bool ignoredIntegrator[ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL];

  DbDeviceTypePtr deviceType;

  // Number of analog channels in the same card - this information
  // comes from the ApplicationCard type (from the cardId attribute),
  // and it is filled out when the database is loaded and configured
  uint32_t numChannelsCard;

  // Pointer to the Channel connected to the device
  DbChannelPtr channel;

  // Array of pointer to bypasses, one for each threshold (max of 32 thresholds).
  // The bypasses are managed by the BypassManager class, when a bypass is added or
  // expires the bypassMask for the AnalogDevice is updated.
  InputBypassPtr bypass[ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL];

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
  uint16_t fastDestinationMask[ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL];

  // For each fastDestinationMask there are 8 sets of power classes -
  // one per each threshold.
  // 4-bit encoded max beam power class for each integrator threshold
  // 4 integrators max, each with size of 8-bits - total of 32 elements
  uint16_t fastPowerClass[ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL * ANALOG_CHANNEL_INTEGRATORS_SIZE];
  uint16_t fastPowerClassInit[ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL * ANALOG_CHANNEL_INTEGRATORS_SIZE];

  DbAnalogDevice();

  uint32_t unlatch(uint32_t mask);
  void update(uint32_t v);
  void update();

  //  void setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer);

  friend std::ostream & operator<<(std::ostream &os, DbAnalogDevice * const analogDevice);
};

typedef shared_ptr<DbAnalogDevice> DbAnalogDevicePtr;
typedef std::map<uint32_t, DbAnalogDevicePtr> DbAnalogDeviceMap;
typedef shared_ptr<DbAnalogDeviceMap> DbAnalogDeviceMapPtr;


/**
 * DbApplicationCard YAML class
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

  // Application Type Card
  DbApplicationTypePtr applicationType;

  // Crate
  DbCratePtr crate;

  // Memory containing firmware configuration buffer
  ApplicationConfigBufferBitSet *applicationConfigBuffer;

  // Input updates - for debugging only
  ApplicationUpdateBufferBitSet *applicationUpdateBuffer;
  ApplicationUpdateBufferFullBitSet *applicationUpdateBufferFull;

  // Each application has a list of analog or digital devices.
  // Only the devices that have 'fast' evaluation need
  // to be in this list - these maps are used to configure
  // the firmware logic
  DbAnalogDeviceMapPtr analogDevices;
  DbDigitalDeviceMapPtr digitalDevices;

  ApplicationUpdateBufferBitSetHalf *wasLowBuffer;
  ApplicationUpdateBufferBitSetHalf *wasHighBuffer;

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
 * DbFaultInput YAML class
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
 * DbBeamClass YAML class
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
 * DbBeamDestination YAML class
 */
class DbBeamDestination : public DbEntry {
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

  DbBeamDestination();

  void setAllowedBeamClass() {
    allowedBeamClass = tentativeBeamClass;
    softwareMitigationBuffer[softwareMitigationBufferIndex] |= ((allowedBeamClass->number & 0xF) << bitShift);

    if (previousAllowedBeamClass->number != allowedBeamClass->number) {
      History::getInstance().logMitigation(id, previousAllowedBeamClass->id, allowedBeamClass->id);
    }

  }

  friend std::ostream & operator<<(std::ostream &os, DbBeamDestination * const beamDestination);
};

typedef shared_ptr<DbBeamDestination> DbBeamDestinationPtr;
typedef std::map<uint32_t, DbBeamDestinationPtr> DbBeamDestinationMap;
typedef shared_ptr<DbBeamDestinationMap> DbBeamDestinationMapPtr;


/**
 * DbAllowedClass YAML class
 */
class DbAllowedClass : public DbEntry {
 public:
  uint32_t beamClassId;
  uint32_t faultStateId;
  uint32_t beamDestinationId;

  // Configured after loading the YAML file
  DbBeamClassPtr beamClass;
  DbBeamDestinationPtr beamDestination;

  DbAllowedClass();

  friend std::ostream & operator<<(std::ostream &os, DbAllowedClass * const allowedClass);
};

typedef shared_ptr<DbAllowedClass> DbAllowedClassPtr;
typedef std::map<uint32_t, DbAllowedClassPtr> DbAllowedClassMap;
typedef shared_ptr<DbAllowedClassMap> DbAllowedClassMapPtr;


/**
 * DbFaultState YAML class
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
  DbAllowedClassMapPtr allowedClasses; // Map of allowed classes (one for each beam destination) for this fault states
  DbDeviceStatePtr deviceState;

  DbFaultState();
  friend std::ostream & operator<<(std::ostream &os, DbFaultState * const digitalFault);
};


/**
 * DbFault YAML class (these are digital faults types)
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

  void unlatch();
  void update(uint32_t v) {
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbFault * const fault);
};

typedef shared_ptr<DbFault> DbFaultPtr;
typedef std::map<uint32_t, DbFaultPtr> DbFaultMap;
typedef shared_ptr<DbFaultMap> DbFaultMapPtr;

/**
 * DbConditionInput YAML class
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
 * DbIgnoreCondition YAML class
 */
class DbIgnoreCondition : public DbEntry {
 public:
  static const uint32_t INVALID_ID = 0xFFFFFFFF;

  uint32_t conditionId;
  uint32_t faultStateId;
  uint32_t analogDeviceId;

  // The ignore condition can be used to ignore a fault state or an analog device
  // The analog device is used to disable a device when beam is blocked upstream, causing
  // faults from no beam
  DbFaultStatePtr faultState;
  DbAnalogDevicePtr analogDevice;

  DbIgnoreCondition();
  friend std::ostream & operator<<(std::ostream &os, DbIgnoreCondition * const input);
};

typedef shared_ptr<DbIgnoreCondition> DbIgnoreConditionPtr;
typedef std::map<uint32_t, DbIgnoreConditionPtr> DbIgnoreConditionMap;
typedef shared_ptr<DbIgnoreConditionMap> DbIgnoreConditionMapPtr;

/**
 * DbCondition YAML class
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
