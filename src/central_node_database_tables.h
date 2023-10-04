/**
 * central_node_database_tables.h
 *
 * C++ class definitions of the MPS configuration database tables.
 * Note - For the classes, most fields are loaded from the YAML,
 * but some are loaded during the configuration. 
 */

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

#include <boost/shared_ptr.hpp>

#define TAB_4  "    " // Used for console output formatting
#define TAB_8  "        " 
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
  uint32_t crateId;
  uint32_t numSlots;
  std::string location;
  std::string rack;
  uint32_t elevation;
  std::string area;
  std::string node;

  DbCrate();
  friend std::ostream & operator<<(std::ostream &os, DbCrate * const crate);
};

typedef boost::shared_ptr<DbCrate> DbCratePtr;
typedef std::map<uint32_t, DbCratePtr> DbCrateMap;
typedef boost::shared_ptr<DbCrateMap> DbCrateMapPtr;

/**
 * DbLinkNode YAML class
 */
class DbLinkNode : public DbEntry {
 public:
  std::string location;
  std::string groupLink;
  uint32_t rxPgp;
  uint32_t lnType;
  uint32_t lnId;
  uint32_t crateId;
  uint32_t groupId;

  DbLinkNode();
  
  friend std::ostream & operator<<(std::ostream &os, DbLinkNode * const linkNode);
};

typedef boost::shared_ptr<DbLinkNode> DbLinkNodePtr;
typedef std::map<uint32_t, DbLinkNodePtr> DbLinkNodeMap;
typedef boost::shared_ptr<DbLinkNodeMap> DbLinkNodeMapPtr;


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

typedef boost::shared_ptr<DbInfo> DbInfoPtr;
typedef std::map<uint32_t, DbInfoPtr> DbInfoMap;
typedef boost::shared_ptr<DbInfoMap> DbInfoMapPtr;

/**
 * DbApplicationType YAML class
 */
class DbApplicationType : public DbEntry {
 public:
  uint32_t numIntegrators;
  uint32_t analogChannelCount;
  uint32_t digitalChannelCount;
  uint32_t softwareChannelCount;
  std::string name;

  DbApplicationType();
  friend std::ostream & operator<<(std::ostream &os, DbApplicationType * const appType);
};

typedef boost::shared_ptr<DbApplicationType> DbApplicationTypePtr;
typedef std::map<uint32_t, DbApplicationTypePtr> DbApplicationTypeMap;
typedef boost::shared_ptr<DbApplicationTypeMap> DbApplicationTypeMapPtr;

class DbApplicationCardInput {
 public:
  DbApplicationCardInput() : fwUpdateBuffer(NULL),  wasLowBufferOffset(999), wasHighBufferOffset(888) {};
  ApplicationUpdateBufferBitSetHalf* getWasLowBuffer();
  ApplicationUpdateBufferBitSetHalf* getWasHighBuffer();

  void setUpdateBuffers(std::vector<uint8_t>* bufPtr, const size_t wasLowBufferOff, const size_t wasHighBufferOff);

  uint32_t getWasLow(int channel);
  uint32_t getWasHigh(int channel);

 private:
  std::vector<uint8_t>* fwUpdateBuffer;
  size_t                wasLowBufferOffset;
  size_t                wasHighBufferOffset;
};

class DbFaultInput;
typedef boost::shared_ptr<DbFaultInput> DbFaultInputPtr;
typedef std::map<uint32_t, DbFaultInputPtr> DbFaultInputMap;
typedef boost::shared_ptr<DbFaultInputMap> DbFaultInputMapPtr;

class DbFaultState;
typedef boost::shared_ptr<DbFaultState> DbFaultStatePtr;
typedef std::map<uint32_t, DbFaultStatePtr> DbFaultStateMap;
typedef boost::shared_ptr<DbFaultStateMap> DbFaultStateMapPtr;

/**
 * DbAnalogChannel YAML class
 */
class DbAnalogChannel : public DbEntry, public DbApplicationCardInput {
 public:
  float offset;
  float slope;
  std::string egu;
  uint32_t integrator;
  uint32_t gain_bay;
  uint32_t gain_channel;
  uint32_t number;
  std::string name;
  float z_location;
  uint32_t auto_reset;
  uint32_t evaluation;
  uint32_t cardId; // FK to DbApplicationCard

  // Pointer to the application card type the channel is connected to
  DbApplicationTypePtr appType;

  // Fault inputs and fault states built after the config is loaded
  DbFaultInputMapPtr faultInputs; 
  DbFaultStateMapPtr faultStates;

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
  bool faultedOffline; //true when app card has app timeout
  bool modeActive; //true when SC mode, false when NC mode


  // Faults from the integrators from this device are bypassed when ignored[integrator]==true
  bool ignoredIntegrator[ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL];

  // Number of analog channels in the same card - this information
  // comes from the ApplicationCard type (from the cardId attribute),
  // and it is filled out when the database is loaded and configured
  uint32_t numChannelsCard;

  // Array of pointer to bypasses, one for each threshold (max of 32 thresholds).
  // The bypasses are managed by the BypassManager class, when a bypass is added or
  // expires the bypassMask for the AnalogChannel is updated.
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

  DbAnalogChannel();

  uint32_t unlatch(uint32_t mask);
  void update(uint32_t v);
  void update();

  //  void setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer);

  friend std::ostream & operator<<(std::ostream &os, DbAnalogChannel * const analogChannel);
};

typedef boost::shared_ptr<DbAnalogChannel> DbAnalogChannelPtr;
typedef std::map<uint32_t, DbAnalogChannelPtr> DbAnalogChannelMap;
typedef boost::shared_ptr<DbAnalogChannelMap> DbAnalogChannelMapPtr;

/**
 * DbDigitalChannel YAML class
 */
class DbDigitalChannel : public DbEntry, public DbApplicationCardInput {
 public:
  std::string z_name;
  std::string o_name;
  uint32_t debounce;
  uint32_t alarm_state;
  uint32_t number; // Channel number
  std::string name;
  float z_location;
  uint32_t auto_reset;
  uint32_t evaluation;
  uint32_t cardId; // FK to DbApplicationCard
  uint32_t faultValue; // Determined during configuration loading
  
  // Channel input value must be read from the Central Node Firmware
  uint32_t wasLowBit;
  uint32_t wasHighBit;
  uint32_t value; // calculated from the faultInput - one or zero.
  uint32_t previousValue;

  // Latched value
  uint32_t latchedValue;

  // Count 'was high'=0 & 'was low'=0
  uint32_t invalidValueCount;

  // Faults from these channels are bypassed when ignored==true
  bool ignored;
  bool faultedOffline; //true when app card has app timeout
  bool modeActive; //true when SC mode, false when NC mode

  // Fault inputs and fault states built after the config is loaded
  DbFaultInputMapPtr faultInputs; 
  DbFaultStateMapPtr faultStates;

  bool configured;

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

  void update(uint32_t v);
  void update();

  DbDigitalChannel();

  friend std::ostream & operator<<(std::ostream &os, DbDigitalChannel * const channel);
};

typedef boost::shared_ptr<DbDigitalChannel> DbDigitalChannelPtr;
typedef std::map<uint32_t, DbDigitalChannelPtr> DbDigitalChannelMap;
typedef boost::shared_ptr<DbDigitalChannelMap> DbDigitalChannelMapPtr;

/**
 * DbApplicationCard YAML class
 */
class DbApplicationCard : public DbEntry {
 public:
  uint32_t number;
  uint32_t slotNumber;
  uint32_t crateId;
  uint32_t applicationTypeId;

  // Configured after loading the YAML file
  
  bool online; // True if received non-zero update last 360Hz update period
  bool modeActive; // True when in SC mode, false when in NC mode.
  bool hasInputs; //True if number of inputs > 0
  bool active; //True when the application card is active, mode irrelevant

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
  DbAnalogChannelMapPtr analogChannels;
  DbDigitalChannelMapPtr digitalChannels;

  void setUpdateBufferPtr(std::vector<uint8_t>* p);
  ApplicationUpdateBufferBitSetHalf* getWasLowBuffer();
  ApplicationUpdateBufferBitSetHalf* getWasHighBuffer();

  DbApplicationCard();

  void writeConfiguration(bool enableTimeout = false);
  void writeDigitalConfiguration();
  void writeAnalogConfiguration();

  void printAnalogConfiguration();

  void configureUpdateBuffers();
  bool updateInputs();
  bool isAnalog();
  bool isDigital();

  std::vector<uint8_t>* getFwUpdateBuffer();
  size_t                getWasLowBufferOffset();
  size_t                getWasHighBufferOffset();

  friend std::ostream & operator<<(std::ostream &os, DbApplicationCard * const appCard);

 private:
  std::vector<uint8_t>* fwUpdateBuffer;
  size_t                wasLowBufferOffset;
  size_t                wasHighBufferOffset;
};

typedef boost::shared_ptr<DbApplicationCard> DbApplicationCardPtr;
typedef std::map<uint32_t, DbApplicationCardPtr> DbApplicationCardMap;
typedef boost::shared_ptr<DbApplicationCardMap> DbApplicationCardMapPtr;

/**
 * DbFaultInput YAML class
 */
class DbFaultInput : public DbEntry, public DbApplicationCardInput {
 public:
  // Values loaded from YAML file
  uint32_t faultId;
  uint32_t channelId; // DbDigitalChannel or DbAnalogChannel
  uint32_t bitPosition;

  // Values calculated at run time
  uint32_t value;

  // A fault input may come from a digital channel or analog channel threshold fault bits
  DbAnalogChannelPtr analogChannel;
  DbDigitalChannelPtr digitalChannel;
  DbFaultStatePtr faultState;

  // Device input value must be read from the Central Node Firmware
  uint32_t wasLowBit;
  uint32_t wasHighBit;
  uint32_t previousValue;

  // Latched value
  uint32_t latchedValue;

  // Count 'was high'=0 & 'was low'=0
  uint32_t invalidValueCount;

  // Pouint32_ter to the bypass for this input
  InputBypassPtr bypass;

  // Set true if this input is used by a fast evaluated device (must be the only input to device)
  bool fastEvaluation;
  bool configured;

  void unlatch();
  void update(uint32_t v);
  void update();

  DbFaultInput();

  friend std::ostream & operator<<(std::ostream &os, DbFaultInput * const faultInput);
};

typedef boost::shared_ptr<DbFaultInput> DbFaultInputPtr;
typedef std::map<uint32_t, DbFaultInputPtr> DbFaultInputMap;
typedef boost::shared_ptr<DbFaultInputMap> DbFaultInputMapPtr;


/**
 * DbBeamClass YAML class
 */
class DbBeamClass : public DbEntry {
 public:
  uint32_t number;
  std::string name;
  uint32_t integrationWindow;
  uint32_t minPeriod;
  uint32_t totalCharge;

  DbBeamClass();
  friend std::ostream & operator<<(std::ostream &os, DbBeamClass * const beamClass);
};

typedef boost::shared_ptr<DbBeamClass>     DbBeamClassPtr;
typedef std::map<uint32_t, DbBeamClassPtr> DbBeamClassMap;
typedef boost::shared_ptr<DbBeamClassMap>  DbBeamClassMapPtr;
typedef std::vector<uint32_t>              DbMitBuffer;

/**
 * DbBeamDestination YAML class
 */
class DbBeamDestination : public DbEntry {
 public:
  std::string name;
  uint16_t destinationMask;
  uint16_t displayOrder;
  uint32_t buffer0DestinationMask;
  uint32_t buffer1DestinationMask;

  // Used after loading the YAML file
  DbBeamClassPtr previousAllowedBeamClass; // beam class set previously
  DbBeamClassPtr allowedBeamClass; // allowed beam class after evaluating faults
  DbBeamClassPtr tentativeBeamClass; // used while faults are being evaluated
  DbBeamClassPtr forceBeamClass; // used when operators want to force a specific beam class
  DbBeamClassPtr softPermit;     // operator SW beam permit or revoke, independent of forceBeamClass
  DbBeamClassPtr maxPermit;     // maximum beam class for 100 MeV operation only.  To be removed later.

  // Memory location where the allowed beam class for this device
  // is written and sent to firmware
  void    setSoftwareMitigationBuffer(DbMitBuffer* bufPtr) { softwareMitigationBuffer = bufPtr; }
  uint8_t softwareMitigationBufferIndex;
  uint8_t bitShift; // define if mitigation uses high/low 4-bits in the softwareMitigationBuffer

  DbBeamDestination();

  void setAllowedBeamClass() {
    uint32_t allowedClassExpand = 0;
    uint8_t shift = 0;
    // If beam destination has a forceBeamClass, then set it
    if (forceBeamClass) {
      if(forceBeamClass->number < tentativeBeamClass->number) {
        tentativeBeamClass = forceBeamClass;
      }
    }
    // If beam destination has a maxPermit, then set it
    if (maxPermit) {
      if(maxPermit->number < tentativeBeamClass->number) {
        tentativeBeamClass = maxPermit;
      }
    }
    // If beam destination has a softPermit, then set it
    if (softPermit) {
      if(softPermit->number < tentativeBeamClass->number) {
        allowedBeamClass = softPermit;
      }
      else {
        allowedBeamClass = tentativeBeamClass;
      }
    }
    else {
      allowedBeamClass = tentativeBeamClass;
    }
    for (int i = 0; i < 8; i++) {
      allowedClassExpand |= ((allowedBeamClass->number & 0xF) << shift);
      shift +=4;
    }
    softwareMitigationBuffer->at(0) |= buffer0DestinationMask & allowedClassExpand;
    softwareMitigationBuffer->at(1) |= buffer1DestinationMask & allowedClassExpand;
  }

  void setForceBeamClass(DbBeamClassPtr beamClass) {
    forceBeamClass = beamClass;
  }

  void resetForceBeamClass() {
    forceBeamClass.reset();
  }

  void setSoftPermit(DbBeamClassPtr beamClass) {
    softPermit = beamClass;
  }

  void resetSoftPermit() {
    softPermit.reset();
  }

  void setMaxPermit(DbBeamClassPtr beamClass) {
    maxPermit = beamClass;
  }

  void resetMaxPermit() {
    maxPermit.reset();
  }

  friend std::ostream & operator<<(std::ostream &os, DbBeamDestination * const beamDestination);

 private:
  DbMitBuffer* softwareMitigationBuffer;
};

typedef boost::shared_ptr<DbBeamDestination> DbBeamDestinationPtr;
typedef std::map<uint32_t, DbBeamDestinationPtr> DbBeamDestinationMap;
typedef boost::shared_ptr<DbBeamDestinationMap> DbBeamDestinationMapPtr;


/**
 * DbAllowedClass YAML class (AKA Mitigation in the sqlite configuration)
 */
class DbAllowedClass : public DbEntry {
 public:
  uint32_t beamClassId;
  uint32_t beamDestinationId;

  // Configured after loading the YAML file
  DbBeamClassPtr beamClass;
  DbBeamDestinationPtr beamDestination;

  DbAllowedClass();

  friend std::ostream & operator<<(std::ostream &os, DbAllowedClass * const allowedClass);
};

typedef boost::shared_ptr<DbAllowedClass> DbAllowedClassPtr;
typedef std::map<uint32_t, DbAllowedClassPtr> DbAllowedClassMap;
typedef boost::shared_ptr<DbAllowedClassMap> DbAllowedClassMapPtr;


/**
 * DbFaultState YAML class
 */
class DbFaultState : public DbEntry {
 public:
  bool defaultState;
  uint32_t mask;
  std::vector<unsigned int> mitigationIds;
  std::string name;
  uint32_t value;
  uint32_t faultId;

  // Configured/Used after loading the YAML file
  bool faulted; // Evaluated based on the status of the deviceState
  bool ignored; // Fault state is ignored if there are valid ignore conditions
  DbAllowedClassMapPtr allowedClasses; // Map of allowed classes (one for each beam destination) for this fault states

  DbFaultState();
  int getIntegrator();
  friend std::ostream & operator<<(std::ostream &os, DbFaultState * const digitalFault);
};

/**
 * DbFault YAML class (these are digital faults types)
 */
class DbFault : public DbEntry {
 public:
  std::string name;
  std::string pv;
  std::vector<unsigned int> ignoreConditionIds;

  // Configured after loading the YAML file
  bool faulted;
  bool faultedDisplay;
  bool ignored;
  bool sendUpdate;
  bool faultedOffline; // True when app card is offline
  bool faultActive; //true when SC mode, false when NC mode
  uint32_t evaluation; // Set according to the input device types
  DbFaultInputMapPtr faultInputs; // A fault may be built by several devices
  uint32_t value; // Calculated from the list of faultInputs
  uint32_t oldValue; // Calculated from the list of faultInputs
  int32_t worstState; //most restrictive device state for this fault
  int32_t displayState;
  DbFaultStateMapPtr faultStates; // Map of fault states for this fault
  DbFaultStatePtr defaultFaultState; // Default fault state if no other fault is active
                                                   // the default state not necessarily is a real fault

  DbFault();

  void update(uint32_t v) {
    oldValue = value;
    value = v;
  }

  friend std::ostream & operator<<(std::ostream &os, DbFault * const fault);
};

typedef boost::shared_ptr<DbFault> DbFaultPtr;
typedef std::map<uint32_t, DbFaultPtr> DbFaultMap;
typedef boost::shared_ptr<DbFaultMap> DbFaultMapPtr;

/**
 * DbIgnoreCondition YAML class
 */
class DbIgnoreCondition : public DbEntry {
 public:
  std::string name;
  std::string description;
  uint32_t value;
  uint32_t digitalChannelId;

  bool state; // State is true when condition is met
  DbFaultMapPtr faults; // Ignore conditions can have >=1 faults
  DbFaultInputMapPtr faultInputs; // Ignore conditions can have >=1 faultInputs

  // The ignore condition can be used to ignore a fault state
  DbDigitalChannelPtr digitalChannel;
  static const uint32_t INVALID_ID = 0xFFFFFFFF;

  DbIgnoreCondition();
  friend std::ostream & operator<<(std::ostream &os, DbIgnoreCondition * const ignoreCondition);
};

typedef boost::shared_ptr<DbIgnoreCondition> DbIgnoreConditionPtr;
typedef std::map<uint32_t, DbIgnoreConditionPtr> DbIgnoreConditionMap;
typedef boost::shared_ptr<DbIgnoreConditionMap> DbIgnoreConditionMapPtr;


#endif
