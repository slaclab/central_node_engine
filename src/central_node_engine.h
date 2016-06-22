#ifndef CENTRAL_NODE_ENGINE_H
#define CENTRAL_NODE_ENGINE_H

#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>

#include <central_node_exception.h>
#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_bypass.h>
#include <central_node_bypass_manager.h>
#include <time_util.h>

using boost::shared_ptr;

class EngineException: public CentralNodeException {
 public:
  explicit EngineException(const char* message) : CentralNodeException(message) {}
  explicit EngineException(const std::string& message) : CentralNodeException(message) {}
  virtual ~EngineException() throw () {}
};

/**
 * Evaluation Cycle:
 * - Read new info from firmware (inputs and mitigation info)
 * - Update values for digital/analog inputs
 * - Update values for digital/analog faults
 * - Check allowed beam at mitigation devices
 * - Request mitigation
 */

class Engine {
 public:
  Engine();

  int loadConfig(std::string yamlFileName);
  int checkFaults();

  friend class EngineTest;
  friend class BypassTest;
  
  void showFaults();
  void showStats();
  void showMitigationDevices();

 protected:
  int updateInputs();
  void setTentativeBeamClass();
  void setAllowedBeamClass();
  void checkDigitalFaults();
  void checkAnalogFaults();

  MpsDbPtr mpsDb;
  BypassManagerPtr bypassManager;

  DbBeamClassPtr highestBeamClass;
  DbBeamClassPtr lowestBeamClass;

  std::stringstream errorStream;
  TimeAverage checkFaultTime;
};

typedef shared_ptr<Engine> EnginePtr;

#endif
