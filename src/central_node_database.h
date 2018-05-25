#ifndef CENTRAL_NODE_DATABASE_H
#define CENTRAL_NODE_DATABASE_H

#include <map>
#include <exception>
#include <iostream>
#include <bitset>
#include <vector>
#include <cstring>
#include <central_node_database_defs.h>
#include <central_node_exception.h>
#include <central_node_bypass.h>
#include <central_node_history.h>
#include <stdint.h>
#include <time_util.h>
#include "timer.h"
#include "buffer.h"

#include <pthread.h>
#include <boost/shared_ptr.hpp>
#include <boost/thread/thread.hpp>

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
  void configureBeamDestinations();

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
  // uint8_t fastUpdateBuffer[APPLICATION_UPDATE_BUFFER_HEADER_SIZE_BYTES +
		// 	   NUM_APPLICATIONS * APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES];

  DataBuffer     fwUpdateBuffer;
  boost::thread  fwUpdateThread;
  void fwUpdateReader();

  uint64_t _fastUpdateTimeStamp;
  uint64_t _diff;
  uint64_t _maxDiff;
  uint32_t _diffCount;

  /**
   * Each destination takes 4-bits for the allowed power class.
   */
  uint32_t softwareMitigationBuffer[NUM_DESTINATIONS / 8];

  Timer<double> _inputUpdateTime;
  // TimeAverage _inputUpdateTime;
  bool _clearUpdateTime;

  Timer<double> _inputDelayTime;
  // TimeAverage _inputDelayTime;
  bool _clearInputDelayTime;

  uint32_t _inputUpdateTimeout;

  /**
   * Mutex to prevent multiple database access
   */
  static pthread_mutex_t _mutex;
  static bool _initialized;

  uint32_t _updateCounter;
  uint32_t _updateTimeoutCounter;

  Timer<double> mitigationTxTime;

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
  DbBeamDestinationMapPtr beamDestinations;
  DbBeamClassMapPtr beamClasses;
  DbAllowedClassMapPtr allowedClasses;
  DbConditionMapPtr conditions;
  DbIgnoreConditionMapPtr ignoreConditions;
  DbConditionInputMapPtr conditionInputs;
  DbInfoMapPtr databaseInfo;

  friend class FirmwareTest;
  friend class Engine;

  // Name of the loaded YAML file
  std::string name;

  // This is initialized by the configure() method, after loading the YAML file
  //  DbFaultStateMapPtr faultStates;

  // MpsDb(DataBuffer* buffer, uint32_t inputUpdateTimeout=3500);
  MpsDb(uint32_t inputUpdateTimeout=3500);
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
  long getMaxInputDelayTime();
  long getAvgInputDelayTime();

  uint64_t getFastUpdateTimeStamp() const { return _fastUpdateTimeStamp; };
  std::vector<uint8_t> getFastUpdateBuffer();

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
