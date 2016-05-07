#ifndef CENTRAL_NODE_DATABASE_H
#define CENTRAL_NODE_DATABASE_H

#include <boost/shared_ptr.hpp>

using boost::shared_ptr;

struct DbEntry {
  int id;

  DbEntry() : id(-1) {
  }

  std::ostream & operator<<(std::ostream &os) {//, DbEntry * const entry) {
    os << " id[" << id << "];";
    return os;
  }
};

/**
 * Crate:
 * - id: '1'
 *   num_slots: '6'
 *   number: '1'
 *   shelf_number: '1'
 */
struct DbCrate : DbEntry {
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
typedef std::vector<DbCratePtr> DbCrateList;
typedef shared_ptr<DbCrateList> DbCrateListPtr;

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
struct DbApplicationType : DbEntry {
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
typedef std::vector<DbApplicationTypePtr> DbApplicationTypeList;
typedef shared_ptr<DbApplicationTypeList> DbApplicationTypeListPtr;


/**
 * ApplicationCard:
 * - crate_id: '1'
 *   id: '1'
 *   number: '1'
 *   slot_number: '2'
 *   type_id: '1'
 */
struct DbApplicationCard : DbEntry {
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
       << "applicationTypeId[" << appCard->applicationTypeId << "]; ";
    return os;
  }
};

typedef shared_ptr<DbApplicationCard> DbApplicationCardPtr;
typedef std::vector<DbApplicationCardPtr> DbApplicationCardList;
typedef shared_ptr<DbApplicationCardList> DbApplicationCardListPtr;

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
struct DbChannel : DbEntry {
  int number;
  int cardId;
  
 DbChannel() : DbEntry(), number(-1), cardId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbChannel * const channel) {
    os << "id[" << channel->id << "]; " 
       << "number[" << channel->number << "]; "
       << "cardId[" << channel->cardId << "]; ";
    return os;
  }
};

typedef shared_ptr<DbChannel> DbChannelPtr;
typedef std::vector<DbChannelPtr> DbChannelList;
typedef shared_ptr<DbChannelList> DbChannelListPtr;

/**
 * DeviceType:
 * - id: '1'
 *   name: Insertion Device
 */
struct DbDeviceType : DbEntry {
  std::string name;
  
 DbDeviceType() : DbEntry(), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceType * const devType) {
    os << "id[" << devType->id << "]; "
       << "name[" << devType->name << "]; ";
    return os;
  }
};

typedef shared_ptr<DbDeviceType> DbDeviceTypePtr;
typedef std::vector<DbDeviceTypePtr> DbDeviceTypeList;
typedef shared_ptr<DbDeviceTypeList> DbDeviceTypeListPtr;

/**
 * DeviceState:
 * - device_type_id: '1'
 *   id: '1'
 *   name: Out
 *   value: '1'
 */
struct DbDeviceState : DbEntry {
  int deviceTypeId;
  int value;
  std::string name;
  
 DbDeviceState() : DbEntry(), deviceTypeId(-1), value(-1), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceState * const devState) {
    os << "id[" << devState->id << "]; "
       << "deviceTypeId[" << devState->deviceTypeId << "]; "
       << "value[" << devState->value << "]; "
       << "name[" << devState->name << "]; ";
    return os;
  }
};

typedef shared_ptr<DbDeviceState> DbDeviceStatePtr;
typedef std::vector<DbDeviceStatePtr> DbDeviceStateList;
typedef shared_ptr<DbDeviceStateList> DbDeviceStateListPtr;

/**
 * DeviceInput:
 * - bit_position: '0'
 *   channel_id: '1'
 *   digital_device_id: '1'
 *   id: '1'
 */
struct DbDeviceInput : DbEntry {
  int bitPosition;
  int channelId;
  int digitalDeviceId;

 DbDeviceInput() : DbEntry(), 
    bitPosition(-1), channelId(-1), digitalDeviceId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDeviceInput * const deviceInput) {
    os << "id[" << deviceInput->id << "]; " 
       << "bitPosition[" << deviceInput->bitPosition << "]; "
       << "channelId[" << deviceInput->channelId << "]; "
       << "digitalDeviceId[" << deviceInput->digitalDeviceId << "]; ";
    return os;
  }
};

typedef shared_ptr<DbDeviceInput> DbDeviceInputPtr;
typedef std::vector<DbDeviceInputPtr> DbDeviceInputList;
typedef shared_ptr<DbDeviceInputList> DbDeviceInputListPtr;

/**
 * DigitalDevice:
 * - device_type_id: '1'
 *   id: '1'
 */
struct DbDigitalDevice : DbEntry {
  int deviceTypeId;
  
 DbDigitalDevice() : DbEntry(), deviceTypeId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDigitalDevice * const digitalDevice) {
    os << "id[" << digitalDevice->id << "]; "
       << "deviceTypeId[" << digitalDevice->deviceTypeId << "]; ";
    return os;
  }
};

typedef shared_ptr<DbDigitalDevice> DbDigitalDevicePtr;
typedef std::vector<DbDigitalDevicePtr> DbDigitalDeviceList;
typedef shared_ptr<DbDigitalDeviceList> DbDigitalDeviceListPtr;

/** 
 * Fault: - description: None id: '1' name: OTR Fault
 */
struct DbFault : DbEntry {
  std::string name;
  std::string description;
  
 DbFault() : DbEntry(), name(""), description("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbFault * const fault) {
    os << "id[" << fault->id << "]; "
       << "name[" << fault->name << "]; "
       << "description[" << fault->description << "]; ";
    return os;
  }
};

typedef shared_ptr<DbFault> DbFaultPtr;
typedef std::vector<DbFaultPtr> DbFaultList;
typedef shared_ptr<DbFaultList> DbFaultListPtr;

/**
 * FaultInput:
 * - bit_position: '0'
 *   device_id: '1'
 *   fault_id: '1'
 *   id: '1'
 */
struct DbFaultInput : DbEntry {
  int faultId;
  int deviceId;
  int bitPosition;
  
 DbFaultInput() : DbEntry(), faultId(-1), deviceId(-1), bitPosition(-1) {
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
typedef std::vector<DbFaultInputPtr> DbFaultInputList;
typedef shared_ptr<DbFaultInputList> DbFaultInputListPtr;

/**
 * DigitalFaultState:
 * - fault_id: '1'
 *   id: '1'
 *   name: Out
 *   value: '1'
 */
struct DbDigitalFaultState : DbEntry {
  int faultId;
  int value;
  std::string name;
  
 DbDigitalFaultState() : DbEntry(), faultId(-1), value(-1), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbDigitalFaultState * const digitalFault) {
    os << "id[" << digitalFault->id << "]; "
       << "faultId[" << digitalFault->faultId << "]; "
       << "value[" << digitalFault->value << "]; "
       << "name[" << digitalFault->name << "]; ";
    return os;
  }
};

typedef shared_ptr<DbDigitalFaultState> DbDigitalFaultStatePtr;
typedef std::vector<DbDigitalFaultStatePtr> DbDigitalFaultStateList;
typedef shared_ptr<DbDigitalFaultStateList> DbDigitalFaultStateListPtr;

/**
 * ThresholdValueMap:
 * - description: Map for generic PICs.
 *   id: '1'
 */
struct DbThresholdValueMap : DbEntry {
  std::string description;
  
 DbThresholdValueMap() : DbEntry(), description("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbThresholdValueMap * const thresValueMap) {
    os << "id[" << thresValueMap->id << "]; "
       << "description[" << thresValueMap->description << "]; ";
    return os;
  }
};

typedef shared_ptr<DbThresholdValueMap> DbThresholdValueMapPtr;
typedef std::vector<DbThresholdValueMapPtr> DbThresholdValueMapList;
typedef shared_ptr<DbThresholdValueMapList> DbThresholdValueMapListPtr;

/**
 * ThresholdValue:
 * - id: '1'
 *   threshold: '0'
 *   threshold_value_map_id: '1'
 *   value: '0.0'
 */
struct DbThresholdValue : DbEntry {
  int threshold;
  int thresholdValueMapId;
  float value;
  
 DbThresholdValue() : DbEntry(), threshold(-1), thresholdValueMapId(-1), value(0.0) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbThresholdValue * const thresValue) {
    os << "id[" << thresValue->id << "]; "
       << "threshold[" << thresValue->threshold << "]; "
       << "thresholdValueMapId[" << thresValue->thresholdValueMapId << "]; "
       << "value[" << thresValue->value << "]; ";

    return os;
  }
};

typedef shared_ptr<DbThresholdValue> DbThresholdValuePtr;
typedef std::vector<DbThresholdValuePtr> DbThresholdValueList;
typedef shared_ptr<DbThresholdValueList> DbThresholdValueListPtr;

/**
 * ThresholdFault:
 * - analog_device_id: '3'
 *   greater_than: 'True'
 *   id: '1'
 *   name: PIC Loss > 1.0
 *   threshold: '1.0'
 */
struct DbThresholdFault : DbEntry {
  std::string name;
  float threshold;
  bool greaterThan;
  int analogDeviceId;
  
 DbThresholdFault() : DbEntry(), name(""), threshold(0.0), greaterThan(true), analogDeviceId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbThresholdFault * const thresFault) {
    os << "id[" << thresFault->id << "]; "
       << "threshold[" << thresFault->threshold << "]; "
       << "greaterThan[" << thresFault->greaterThan << "]; "
       << "analogDeviceId[" << thresFault->analogDeviceId << "]; "
       << "name[" << thresFault->name << "]; ";
    return os;
  }
};

typedef shared_ptr<DbThresholdFault> DbThresholdFaultPtr;
typedef std::vector<DbThresholdFaultPtr> DbThresholdFaultList;
typedef shared_ptr<DbThresholdFaultList> DbThresholdFaultListPtr;


/**
 * ThresholdFaultState:
 * - id: '8'
 *   threshold_fault_id: '1'
 */
struct DbThresholdFaultState : DbEntry {
  int thresholdFaultId;
  
 DbThresholdFaultState() : DbEntry(), thresholdFaultId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbThresholdFaultState * const thresFaultState) {
    os << "id[" << thresFaultState->id << "]; "
       << "thresholdFaultId[" << thresFaultState->thresholdFaultId << "]; ";
    return os;
  }
};

typedef shared_ptr<DbThresholdFaultState> DbThresholdFaultStatePtr;
typedef std::vector<DbThresholdFaultStatePtr> DbThresholdFaultStateList;
typedef shared_ptr<DbThresholdFaultStateList> DbThresholdFaultStateListPtr;


/**
 * MitigationDevice:
 * - id: '1'
 *   name: Gun
 */
struct DbMitigationDevice : DbEntry {
  std::string name;
  
 DbMitigationDevice() : DbEntry(), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbMitigationDevice * const mitigationDevice) {
    os << "id[" << mitigationDevice->id << "]; "
       << "name[" << mitigationDevice->name << "]; ";
    return os;
  }
};

typedef shared_ptr<DbMitigationDevice> DbMitigationDevicePtr;
typedef std::vector<DbMitigationDevicePtr> DbMitigationDeviceList;
typedef shared_ptr<DbMitigationDeviceList> DbMitigationDeviceListPtr;


/**
 * BeamClass:
 * - id: '1'
 *   name: Class 1
 *   number: '1'
 */
struct DbBeamClass : DbEntry {
  int number;
  std::string name;
  
 DbBeamClass() : DbEntry(), number(-1), name("") {
  }

  friend std::ostream & operator<<(std::ostream &os, DbBeamClass * const beamClass) {
    os << "id[" << beamClass->id << "]; "
       << "number[" << beamClass->number << "]; "
       << "name[" << beamClass->name << "]; ";
    return os;
  }
};

typedef shared_ptr<DbBeamClass> DbBeamClassPtr;
typedef std::vector<DbBeamClassPtr> DbBeamClassList;
typedef shared_ptr<DbBeamClassList> DbBeamClassListPtr;


/**
 * AllowedClass:
 * - beam_class_id: '1'
 *   fault_state_id: '1'
 *   id: '1'
 *   mitigation_device_id: '1'
 */
struct DbAllowedClass : DbEntry {
  int beamClassId;
  int faultStateId;
  int mitigationDeviceId;
  
 DbAllowedClass() : DbEntry(), beamClassId(-1), faultStateId(-1), mitigationDeviceId(-1) {
  }

  friend std::ostream & operator<<(std::ostream &os, DbAllowedClass * const allowedClass) {
    os << "id[" << allowedClass->id << "]; "
       << "beamClassId[" << allowedClass->beamClassId << "]; "
       << "faultStateId[" << allowedClass->faultStateId << "]; "
       << "mitigationDeviceId[" << allowedClass->mitigationDeviceId << "]; ";
    return os;
  }
};

typedef shared_ptr<DbAllowedClass> DbAllowedClassPtr;
typedef std::vector<DbAllowedClassPtr> DbAllowedClassList;
typedef shared_ptr<DbAllowedClassList> DbAllowedClassListPtr;


/*****************************/


typedef struct MpsDb {
  DbCrateListPtr crates;
  DbApplicationTypeListPtr applicationTypes;
  DbApplicationCardListPtr applicationCards;
  DbChannelListPtr digitalChannels;
  DbChannelListPtr analogChannels;
  DbDeviceTypeListPtr deviceTypes;
  DbDeviceStateListPtr deviceStates;
  DbDigitalDeviceListPtr digitalDevices;
  DbDeviceInputListPtr deviceInputs;
  DbFaultListPtr faults;
  DbFaultInputListPtr faultInputs;
  DbDigitalFaultStateListPtr digitalFaultStates;
  DbThresholdValueMapListPtr thresholdValueMaps;
  DbThresholdValueListPtr thresholdValues;
  DbThresholdFaultListPtr thresholdFaults;
  DbThresholdFaultStateListPtr thresholdFaultStates;
  DbMitigationDeviceListPtr mitigationDevices;
  DbBeamClassListPtr beamClasses;
  DbAllowedClassListPtr allowedClasses;

  friend std::ostream & operator<<(std::ostream &os, MpsDb * const mpsDb) {
    os << "Crates: " << std::endl;
    for (DbCrateList::iterator it = mpsDb->crates->begin(); 
	 it != mpsDb->crates->end(); ++it) {
      os << "  " << (*it) << std::endl;
    }

    os << "ApplicationTypes: " << std::endl;
    for (DbApplicationTypeList::iterator it = mpsDb->applicationTypes->begin(); 
	 it != mpsDb->applicationTypes->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "ApplicationCards: " << std::endl;
    for (DbApplicationCardList::iterator it = mpsDb->applicationCards->begin(); 
	 it != mpsDb->applicationCards->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "DigitalChannels: " << std::endl;
    for (DbChannelList::iterator it = mpsDb->digitalChannels->begin(); 
	 it != mpsDb->digitalChannels->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "AnalogChannels: " << std::endl;
    for (DbChannelList::iterator it = mpsDb->analogChannels->begin(); 
	 it != mpsDb->analogChannels->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "DeviceTypes: " << std::endl;
    for (DbDeviceTypeList::iterator it = mpsDb->deviceTypes->begin(); 
	 it != mpsDb->deviceTypes->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "DeviceStates: " << std::endl;
    for (DbDeviceStateList::iterator it = mpsDb->deviceStates->begin(); 
	 it != mpsDb->deviceStates->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "DigitalDevices: " << std::endl;
    for (DbDigitalDeviceList::iterator it = mpsDb->digitalDevices->begin(); 
	 it != mpsDb->digitalDevices->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "DeviceInputs: " << std::endl;
    for (DbDeviceInputList::iterator it = mpsDb->deviceInputs->begin(); 
	 it != mpsDb->deviceInputs->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "Faults: " << std::endl;
    for (DbFaultList::iterator it = mpsDb->faults->begin(); 
	 it != mpsDb->faults->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "FaultInputs: " << std::endl;
    for (DbFaultInputList::iterator it = mpsDb->faultInputs->begin(); 
	 it != mpsDb->faultInputs->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "DigitalFaultStates: " << std::endl;
    for (DbDigitalFaultStateList::iterator it = mpsDb->digitalFaultStates->begin(); 
	 it != mpsDb->digitalFaultStates->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "ThresholdValueMaps: " << std::endl;
    for (DbThresholdValueMapList::iterator it = mpsDb->thresholdValueMaps->begin(); 
	 it != mpsDb->thresholdValueMaps->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "ThresholdValues: " << std::endl;
    for (DbThresholdValueList::iterator it = mpsDb->thresholdValues->begin(); 
	 it != mpsDb->thresholdValues->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "ThresholdFaults: " << std::endl;
    for (DbThresholdFaultList::iterator it = mpsDb->thresholdFaults->begin(); 
	 it != mpsDb->thresholdFaults->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "ThresholdFaultStates: " << std::endl;
    for (DbThresholdFaultStateList::iterator it = mpsDb->thresholdFaultStates->begin(); 
	 it != mpsDb->thresholdFaultStates->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "MitigationDevices: " << std::endl;
    for (DbMitigationDeviceList::iterator it = mpsDb->mitigationDevices->begin(); 
	 it != mpsDb->mitigationDevices->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "BeamClasss: " << std::endl;
    for (DbBeamClassList::iterator it = mpsDb->beamClasses->begin(); 
	 it != mpsDb->beamClasses->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }

    os << "AllowedClasss: " << std::endl;
    for (DbAllowedClassList::iterator it = mpsDb->allowedClasses->begin(); 
	 it != mpsDb->allowedClasses->end(); ++it) {
      os << "  " <<  (*it) << std::endl;
    }


    return os;
  }
} MpsDb;

#endif
