#ifndef CENTRAL_NODE_ENGINE_H
#define CENTRAL_NODE_ENGINE_H

#include <iostream>
#include <sstream>
#include <pthread.h>
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
 private:
  Engine();
  Engine(Engine const &);
  ~Engine();
  void operator=(Engine const &);
  bool initialized;
  pthread_t _engineThread;

 public:
  int loadConfig(std::string yamlFileName);
  int checkFaults();
  bool isInitialized();

  friend class EngineTest;
  friend class BypassTest;
  friend class FirmwareTest;
  
  void showFaults();
  void showStats();
  void showMitigationDevices();
  void showDeviceInputs();
  void showFirmware();

  MpsDbPtr getCurrentDb();
  BypassManagerPtr getBypassManager();

 protected:
  void newDb();
  void updateInputs();
  void mitigate();

  void setTentativeBeamClass();
  void setAllowedBeamClass();

  void evaluateFaults();
  void evaluateIgnoreConditions();
  void mitgate();

  MpsDbPtr mpsDb;
  BypassManagerPtr bypassManager;

  DbBeamClassPtr highestBeamClass;
  DbBeamClassPtr lowestBeamClass;

  std::stringstream errorStream;
  TimeAverage checkFaultTime;

 public:
  static Engine &getInstance() {
    static Engine instance;
    return instance;
  }

  void startUpdateThread();
  static void *engineThread(void *arg);
};

typedef shared_ptr<Engine> EnginePtr;

#endif
