#ifndef CENTRAL_NODE_DATABASE_H
#define CENTRAL_NODE_DATABASE_H

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
  void configureMitigationDevices();

  /**
   * Memory that holds the firmware/hardware database configuration.
   * Every 0x100 chunk is reserved for an application configuration.
   * Even though there are 2048 bits on every chunk, the digital
   * configuration takes 1343 bits and the analog configuration takes
   * 1151 bits.
   */
  uint8_t fastConfigurationBuffer[NUM_APPLICATIONS * APPLICATION_CONFIG_BUFFER_SIZE_BYTES];

  /**
   * Memory that receives input updates from firmware/hardware. There are
   * up to 384 inputs bits per application, 2-bit per input indicating 
   * a 'was low' and 'was high' status.
   *
   * The bits from 0 to 191 are the 'was low' status, and bits 192 to 383
   * are the 'was high' status.
   */
  uint8_t fastUpdateBuffer[APPLICATION_UPDATE_BUFFER_HEADER_SIZE_BYTES + 
			   NUM_APPLICATIONS * APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES];

  /** 
   * Each destination takes 4-bits for the allowed power class.
   */
  uint32_t softwareMitigationBuffer[NUM_DESTINATIONS / 8];

  TimeAverage _inputUpdateTime;
  bool _clearUpdateTime;

  /**
   * Mutex to prevent multiple database access
   */
  pthread_mutex_t mutex;

  uint32_t _updateCounter;
  
 public:
  DbBeamClassPtr lowestBeamClass;
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

  friend class FirmwareTest;

  // Name of the loaded YAML file
  std::string name;

  // This is initialized by the configure() method, after loading the YAML file
  //  DbFaultStateMapPtr faultStates; 

  MpsDb();
  ~MpsDb();
  int load(std::string yamlFile);
  void configure();

  void lock();
  void unlock();

  void showFastUpdateBuffer(uint32_t begin, uint32_t size);
  void showFaults();
  void showFault(DbFaultPtr fault);
  void showMitigation();
  void showInfo();

  void writeFirmwareConfiguration();
  bool updateInputs();
  void unlatchAll();
  void clearMitigationBuffer();
  void mitigate();

  void clearUpdateTime();
  long getMaxUpdateTime();
  long getAvgUpdateTime();

  uint8_t *getFastUpdateBuffer() {
    return &fastUpdateBuffer[0];
  }
  
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

  friend std::ostream & operator<<(std::ostream &os, MpsDb * const mpsDb);
};

typedef shared_ptr<MpsDb> MpsDbPtr;

#endif
