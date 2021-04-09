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

#include <boost/shared_ptr.hpp>
#include <boost/atomic.hpp>
#include <thread>
#include <mutex>

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

  DataBuffer<uint8_t>     fwUpdateBuffer;
  static const uint32_t          fwUpdateBuferSize = APPLICATION_UPDATE_BUFFER_HEADER_SIZE_BYTES + NUM_APPLICATIONS * APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES;

  boost::atomic<bool>     run;

  std::thread fwUpdateThread;
  std::thread fwPCChangeThread;
  std::thread updateInputThread;
  std::thread mitigationThread;

  void fwUpdateReader();
  void updateInputs();
  void mitigationWriter();

  void fwPCChangeReader();
  void printPCChangeLastPacketInfo() const;

  bool                    inputsUpdated;
  std::mutex              inputsUpdatedMutex;
  std::condition_variable inputsUpdatedCondVar;

  uint64_t _fastUpdateTimeStamp;
  uint64_t _diff;
  uint64_t _maxDiff;
  uint32_t _diffCount;

  /**
   * Each destination takes 4-bits for the allowed power class.
   */
  DataBuffer<uint32_t> softwareMitigationBuffer;

  Timer<double> _inputUpdateTime;
  bool _clearUpdateTime;

  Timer<double> fwUpdateTimer;

  uint32_t _inputUpdateTimeout;

  /**
   * Mutex to prevent multiple database access
   */
  static std::mutex _mutex;
  static bool _initialized;

  uint32_t _updateCounter;
  uint32_t _updateTimeoutCounter;

  // Power class change messages related variables
  std::size_t _pcChangeCounter;         // Number of valid packet received
  std::size_t _pcChangeBadSizeCounter;  // Number of received packets with unexpected size
  std::size_t _pcChangeOutOrderCounter; // Number of packets received out of ourder (based on tag)
  std::size_t _pcChangeLossCounter;     // Number of loss packets (based on tag)
  std::size_t _pcChangeSameTagCounter;  // Number of packets received with the same tag
  bool        _pcChangeFirstPacket;     // Flag to indicate the first received packet
  bool        _pcChangeDebug;           // Debug flag. When true, the content of each received packet will be printed
  uint16_t    _pcChangeTag;             // Last received message tag
  uint16_t    _pcChangeFlags;           // Last received message flags
  uint16_t    _pcChangeTimeStamp;       // Last received message timestamp
  uint64_t    _pcChangePowerClass;      // Last received message power class
  std::vector<std::size_t> _pcFlagsCounters;
  // Power class ttansition counters. One counter for each power class, for each destination.
  // Each word counts the number of transition to that power class/destination combination.
  std::size_t _pcCounters[NUM_DESTINATIONS][1<<POWER_CLASS_BIT_SIZE];

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

  MpsDb(uint32_t inputUpdateTimeout=3500);
  ~MpsDb();
  int load(std::string yamlFile);
  void configure();

  std::mutex *getMutex() { return &_mutex; };

  void showFastUpdateBuffer(uint32_t begin, uint32_t size);
  void showFaults();
  void showFault(DbFaultPtr fault);
  void showMitigation();
  void showInfo();
  void printPCChangeInfo() const;
  void PCChangeSetDebug(bool debug);
  void printPCCounters() const;

  void forceBeamDestination(uint32_t beamDestinationId, uint32_t beamClassId=CLEAR_BEAM_CLASS);
  void writeFirmwareConfiguration(bool forceAomAllow = false);
  void unlatchAll();
  void clearMitigationBuffer();

  void clearUpdateTime();
  long getMaxUpdateTime();
  long getAvgUpdateTime();
  long getMaxFwUpdatePeriod();
  long getAvgFwUpdatePeriod();

  uint64_t getFastUpdateTimeStamp() const { return _fastUpdateTimeStamp; };
  std::vector<uint8_t> getFastUpdateBuffer();

  //  mpsDb->printMap<DbConditionMapPtr, DbConditionMap::iterator>(os, mpsDb->conditions, "Conditions");

  bool                     isInputReady()          const { return inputsUpdated;         };
  std::mutex*              getInputUpdateMutex()         { return &inputsUpdatedMutex;   };
  std::condition_variable* getInputUpdateCondVar()       { return &inputsUpdatedCondVar; };
  void                     inputProcessed()              { inputsUpdated = false;        };

  bool                     isMitBufferWriteReady() const { return softwareMitigationBuffer.isWriteReady(); };
  std::mutex*              getMitBufferMutex()           { return softwareMitigationBuffer.getMutex();     };
  std::condition_variable* getMitBufferCondVar()         { return softwareMitigationBuffer.getCondVar();   };
  void                     mitBufferDoneWriting()        { softwareMitigationBuffer.doneWriting();         };

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

typedef boost::shared_ptr<MpsDb> MpsDbPtr;

#endif
