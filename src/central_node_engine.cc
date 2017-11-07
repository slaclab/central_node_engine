#include <central_node_engine.h>
#include <central_node_firmware.h>
#include <central_node_history.h>

#include <stdio.h>
#include <stdint.h>
#include <sched.h>
#include <sys/mman.h>

#include <log.h>
#include <log_wrapper.h>

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
using namespace easyloggingpp;
static Logger *engineLogger;
#endif 

pthread_mutex_t Engine::_engineMutex;
bool            Engine::_evaluate;
uint32_t        Engine::_rate;
uint32_t        Engine::_updateCounter;
time_t          Engine::_startTime;

Engine::Engine() :
  _initialized(false),
  _debugCounter(0),
  _checkFaultTime(5, "Evaluation only time"),
  _clearCheckFaultTime(false),
  _evaluationCycleTime(5, "Evaluation Cycle time"),
  _clearEvaluationCycleTime(false) {
#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
  engineLogger = Loggers::getLogger("ENGINE");
  LOG_TRACE("ENGINE", "Created Engine");
#endif

  Engine::_evaluate = false;
  Engine::_rate = 0;
  Engine::_updateCounter = 0;
  Engine::_startTime = 0;

  int ret = pthread_mutex_init(&Engine::_engineMutex, NULL);
  if (0 != ret) {
    throw(DbException("ERROR: Engine::Engine() failed to initialize engine mutex."));
  }

  // Lock memory
  if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
    perror("mlockall failed");
    std::cerr << "WARN: Engine::Engine() mlockall failed." << std::endl;
  }
}

Engine::~Engine() {
#if defined(LOG_ENABLED)
  //  Firmware::getInstance().showStats();
#endif
  // Stop the MPS before quitting
  Firmware::getInstance().setSoftwareEnable(false);
  Firmware::getInstance().setEnable(false);

  _checkFaultTime.show();
}

MpsDbPtr Engine::getCurrentDb() {
  return _mpsDb;
}

BypassManagerPtr Engine::getBypassManager() {
  return _bypassManager;
}

void Engine::newDb() {
  MpsDb *db = new MpsDb();
  _mpsDb = shared_ptr<MpsDb>(db);
}

int Engine::reloadConfig() {
  pthread_mutex_lock(&_engineMutex);

  // If updateThread active, then stop it first
  if (_evaluate) {
    _evaluate = false;
    pthread_mutex_unlock(&_engineMutex);
    threadJoin();
    pthread_mutex_lock(&_engineMutex);
  }

  _evaluate = false;

  _mpsDb->lock();
  _mpsDb->writeFirmwareConfiguration();
  _mpsDb->unlock();

  _evaluate = true;
  pthread_mutex_unlock(&_engineMutex);

  startUpdateThread();

  return 0;
}

// This is called when analog devices need to be ignored (and un-ignored)
int Engine::reloadConfigFromIgnore() {
  Firmware::getInstance().setSoftwareEnable(false);
  Firmware::getInstance().setEnable(false);

  _mpsDb->lock();
  _mpsDb->writeFirmwareConfiguration();
  _mpsDb->unlock();

  Firmware::getInstance().setEnable(true);
  Firmware::getInstance().softwareClear();
  Firmware::getInstance().setSoftwareEnable(true);

  return 0;
}

int Engine::loadConfig(std::string yamlFileName) {
  pthread_mutex_lock(&_engineMutex);

  // First stop the MPS
  Firmware::getInstance().setSoftwareEnable(false);
  Firmware::getInstance().setEnable(false);

  // If updateThread active, then stop it first
  if (_evaluate) {
    _evaluate = false;
    pthread_mutex_unlock(&_engineMutex);
    threadJoin();
    pthread_mutex_lock(&_engineMutex);
  }  

  _evaluate = false;

  MpsDb *db = new MpsDb();
  shared_ptr<MpsDb> mpsDb = shared_ptr<MpsDb>(db);

  // Move this for later, so _mpsDb is not overwritten 
  //  _mpsDb = shared_ptr<MpsDb>(db);
  //  _mpsDb->lock();
  mpsDb->lock(); // the database mutex is static - is the same for all instances

  if (mpsDb->load(yamlFileName) != 0) {
    _errorStream.str(std::string());
    _errorStream << "ERROR: Failed to load yaml database ("
		<< yamlFileName << ")";
    mpsDb->unlock();
    pthread_mutex_unlock(&_engineMutex);
    throw(EngineException(_errorStream.str()));
  }

  LOG_TRACE("ENGINE", "YAML Database loaded from " << yamlFileName);

  // When the first yaml configuration file is loaded the bypassMap
  // must be created.  
  //
  // TODO/Database/Important: the bypasses are created for each device input and
  // analog channel in the database, this happens for the first time
  // a database is loaded. If there are subsequent database changes
  // they must have the same device inputs and analog channels.
  // Currently if the number of channels differ between the loaded
  // databases the central node engine must be restarted.
  if (!_bypassManager) {
    _bypassManager = BypassManagerPtr(new BypassManager());
    _bypassManager->createBypassMap(mpsDb);
    _bypassManager->startBypassThread();
  }

  LOG_TRACE("ENGINE", "BypassManager created");

  try {
    mpsDb->configure();
  } catch (DbException e) {
    mpsDb->unlock();
    pthread_mutex_unlock(&_engineMutex);
    throw e;
  }

  LOG_TRACE("ENGINE", "MPS Database configured from YAML");

  // Assign bypass to each digital/analog input
  try {
    _bypassManager->assignBypass(mpsDb);
  } catch (CentralNodeException e) {
    mpsDb->unlock();
    pthread_mutex_unlock(&_engineMutex);
    throw e;
  }

  // Now that the database has been loaded and configured
  // successfully, assign to _mpsDb shared_ptr
  _mpsDb = mpsDb;//shared_ptr<MpsDb>(db);

  // Find the lowest/highest BeamClasses - used when checking faults
  uint32_t num = 0;
  uint32_t lowNum = 100;
  for (DbBeamClassMap::iterator beamClass = _mpsDb->beamClasses->begin();
       beamClass != _mpsDb->beamClasses->end(); ++beamClass) {
    if ((*beamClass).second->number > num) {
      _highestBeamClass = (*beamClass).second;
      num = (*beamClass).second->number;
    }
    if ((*beamClass).second->number < lowNum) {
      _lowestBeamClass = (*beamClass).second;
      lowNum = (*beamClass).second->number;
    }
  }

  _mpsDb->writeFirmwareConfiguration();
  _mpsDb->unlock();

  LOG_TRACE("ENGINE", "Lowest beam class found: " << _lowestBeamClass->number);
  LOG_TRACE("ENGINE", "Highest beam class found: " << _highestBeamClass->number);

  _initialized = true;

  _evaluate = true;
  //  pthread_cond_signal(&_engineCondition);
  pthread_mutex_unlock(&_engineMutex);

  startUpdateThread();

  return 0;
}

bool Engine::isInitialized() {
  return _initialized;
}

/**
 * Prior to check digital/analog faults assign the highest beam class available
 * as tentativeBeamClass for all mitigation devices. Also temporarily sets the
 * allowedBeamClass as the lowest beam class available.
 *
 * As faults are detected the tentativeBeamClass gets lowered accordingly.
 */
void Engine::setTentativeBeamClass() {
  // Assigns _highestBeamClass as tentativeBeamClass for all MitigationDevices
  for (DbMitigationDeviceMap::iterator device = _mpsDb->mitigationDevices->begin();
       device != _mpsDb->mitigationDevices->end(); ++device) {
    (*device).second->tentativeBeamClass = _highestBeamClass;
    (*device).second->previousAllowedBeamClass = (*device).second->allowedBeamClass; // for history purposes
    (*device).second->allowedBeamClass = _lowestBeamClass;
    LOG_TRACE("ENGINE", (*device).second->name << " tentative class set to: "
	      << (*device).second->tentativeBeamClass->number
	      << "; allowed class set to: "
	      << (*device).second->allowedBeamClass->number);
  }
}

/**
 * After checking the faults move the final tentativeBeamClass into the 
 * allowedBeamClass for the mitigation devices.
 */
void Engine::setAllowedBeamClass() {
  for (DbMitigationDeviceMap::iterator it = _mpsDb->mitigationDevices->begin(); 
       it != _mpsDb->mitigationDevices->end(); ++it) {
    (*it).second->setAllowedBeamClass();
    //    (*it).second->allowedBeamClass = (*it).second->tentativeBeamClass;
    LOG_TRACE("ENGINE", (*it).second->name << " allowed class set to "
	      << (*it).second->allowedBeamClass->number);
  }
}

/**
 * First goes through the list of DigitalDevices and updates the current state.
 * Second updates the FaultStates for each Fault.
 * At the end of this method all Digital Faults will have updated values,
 * i.e. the field 'faulted' gets updated based on the current machine state. 
 *
 * TODO: bypass mask and values accessed by multiple threads
 */
void Engine::evaluateFaults() {
  uint32_t deviceStateId = 0;

  bool faulted = false;

  // Update digital device values based on individual inputs
  // The individual inputs come from independent digital inputs,
  // this does not apply to analog devices, where the input
  // come from a single channel (with multiple bits in it)
  for (DbDigitalDeviceMap::iterator device = _mpsDb->digitalDevices->begin(); 
       device != _mpsDb->digitalDevices->end(); ++device) {
    uint32_t deviceValue = 0;
    LOG_TRACE("ENGINE", "Getting inputs for " << (*device).second->name << " device"
	      << ", there are " << (*device).second->inputDevices->size()
	      << " inputs");
    for (DbDeviceInputMap::iterator input = (*device).second->inputDevices->begin(); 
	 input != (*device).second->inputDevices->end(); ++input) {
      uint32_t inputValue = 0;
      // Check if the input has a valid bypass value
      if ((*input).second->bypass->status == BYPASS_VALID) {
	inputValue = (*input).second->bypass->value;
	LOG_TRACE("ENGINE", (*device).second->name << " bypassing input value to "
		  << (*input).second->bypass->value << " (actual value is "
		  << (*input).second->value << ")");
      }
      else {
	inputValue = (*input).second->value;
      }
      inputValue <<= (*input).second->bitPosition;
      deviceValue |= inputValue;
    }
    (*device).second->update(deviceValue);
    LOG_TRACE("ENGINE", (*device).second->name << " current value " << std::hex << deviceValue << std::dec);
  }  

  // At this point all digital devices have an updated value

  // Update digital & analog Fault values and MitigationDevice allowed class
  for (DbFaultMap::iterator fault = _mpsDb->faults->begin();
       fault != _mpsDb->faults->end(); ++fault) {
    LOG_TRACE("ENGINE", (*fault).second->name << " updating fault values");

    // First calculate the digital Fault value from its one or more digital device inputs
    uint32_t faultValue = 0;
    for (DbFaultInputMap::iterator input = (*fault).second->faultInputs->begin();
	 input != (*fault).second->faultInputs->end(); ++input) {
      uint32_t inputValue = 0;
      if ((*input).second->digitalDevice) {
	inputValue = (*input).second->digitalDevice->value;
      }
      else {
	// Check if there is an active bypass for analog device
	LOG_TRACE("ENGINE", (*input).second->analogDevice->name << " bypassMask=" << std::hex <<
		  (*input).second->analogDevice->bypassMask << ", value=" <<
		  (*input).second->analogDevice->value << std::dec);
	inputValue = (*input).second->analogDevice->value & (*input).second->analogDevice->bypassMask;
      }
      faultValue |= (inputValue << (*input).second->bitPosition);
      LOG_TRACE("ENGINE", (*fault).second->name << " current value " << std::hex << faultValue
		<< ", input value " << inputValue << std::dec << " bit pos "
		<< (*input).second->bitPosition);
    }
    (*fault).second->update(faultValue);
    bool oldFaultValue = (*fault).second->faulted;
    (*fault).second->faulted = false; // Clear the fault - in case it was faulted before
    LOG_TRACE("ENGINE", (*fault).second->name << " current value " << std::hex << faultValue << std::dec);

    // Now that a Fault has a new value check if it is in the FaultStates list,
    // and update the allowedBeamClass for the MitigationDevices
    for (DbFaultStateMap::iterator state = (*fault).second->faultStates->begin();
	 state != (*fault).second->faultStates->end(); ++state) {
      (*state).second->ignored = false; // Mark not ignored - the ignore logic is evaluated later
      uint32_t maskedValue = faultValue & (*state).second->deviceState->mask;
      LOG_TRACE("ENGINE", (*fault).second->name << ", checking fault state ["
		<< (*state).second->id << "]: "
		<< "masked value is " << std::hex << maskedValue << " (mask="
		<< (*state).second->deviceState->mask << ")" << std::dec);
      if ((*state).second->deviceState->value == maskedValue) {
	deviceStateId = (*state).second->deviceState->id;
	(*state).second->faulted = true; // Set input faulted field
	(*fault).second->faulted = true; // Set fault faulted field
	(*fault).second->faultLatched = true;
	faulted = true; // Signal that at least one state is faulted
	LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value=" 
		  << faultValue << ", masked=" << maskedValue
		  << " (fault state="
		  << (*state).second->deviceState->name
		  << ", value=" << (*state).second->deviceState->value << ")");
      }
      else {
	(*state).second->faulted = false;
      }
    }

    // If there are no faults, then enable the default - if there is one
    if (!faulted && (*fault).second->defaultFaultState) {
      (*fault).second->defaultFaultState->faulted = true;
      deviceStateId = (*fault).second->defaultFaultState->deviceState->id;
      LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value=" 
		<< faultValue << " (Default) fault state="
		<< (*fault).second->defaultFaultState->deviceState->name);
    }

    if (oldFaultValue != (*fault).second->faulted) {
      History::getInstance().logFault((*fault).second->id, oldFaultValue,
				      (*fault).second->faulted, deviceStateId);
    }
    deviceStateId = 0;
  }
}

// Check if there are valid ignore conditions. If changes in ignore condition
// requires firmware reconfiguration, then return true.
bool Engine::evaluateIgnoreConditions() {
  bool reload = false;
  // Calculate state of conditions
  for (DbConditionMap::iterator condition = _mpsDb->conditions->begin();
       condition != _mpsDb->conditions->end(); ++condition) {
    uint32_t conditionValue = 0;
    for (DbConditionInputMap::iterator input = (*condition).second->conditionInputs->begin();
	 input != (*condition).second->conditionInputs->end(); ++input) {
      uint32_t inputValue = 0;
      if ((*input).second->faultState) {
 	if ((*input).second->faultState->faulted) {
	  inputValue = 1;
	}
      }
      conditionValue |= (inputValue << (*input).second->bitPosition);
      LOG_TRACE("ENGINE", "Condition " << (*condition).second->name << " current value " << std::hex << conditionValue
		<< ", input value " << inputValue << std::dec << " bit pos "
		<< (*input).second->bitPosition);
    }
    

    // 'mask' is the condition value that needs to be matched in order to ignore faults
    bool newConditionState = false;
    if ((*condition).second->mask == conditionValue) {
      newConditionState = true;
    }
    (*condition).second->state = newConditionState;
    LOG_TRACE("ENGINE",  "Condition " << (*condition).second->name << " is " << (*condition).second->state);

    for (DbIgnoreConditionMap::iterator ignoreCondition = (*condition).second->ignoreConditions->begin();
	 ignoreCondition != (*condition).second->ignoreConditions->end(); ++ignoreCondition) {
      if ((*ignoreCondition).second->faultState) {
	LOG_TRACE("ENGINE",  "Ignoring fault state [" << (*ignoreCondition).second->faultStateId << "]"
		  << ", state=" << (*condition).second->state);
	(*ignoreCondition).second->faultState->ignored = (*condition).second->state;
      }
      else {
	if ((*ignoreCondition).second->analogDevice) {
	  LOG_TRACE("ENGINE",  "Ignoring analog device [" << (*ignoreCondition).second->analogDeviceId << "]"
		    << ", state=" << (*condition).second->state);
	  if ((*ignoreCondition).second->analogDevice->ignored != (*condition).second->state) {
	    reload = true; // reload configuration!
	  }
	  (*ignoreCondition).second->analogDevice->ignored = (*condition).second->state;
	}
      }
    }
  }
  return reload;
}

void Engine::mitigate() {
  // Update digital Fault values and MitigationDevice allowed class
  for (DbFaultMap::iterator fault = _mpsDb->faults->begin();
       fault != _mpsDb->faults->end(); ++fault) {
    // Mitigate only those faults that are the result of slow evaluation
    if ((*fault).second->evaluation == SLOW_EVALUATION) {
      for (DbFaultStateMap::iterator state = (*fault).second->faultStates->begin();
	   state != (*fault).second->faultStates->end(); ++state) {
	if ((*state).second->faulted && !(*state).second->ignored) {
	  LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value=" 
		    << (*fault).second->value
		    << " (fault state="
		    << (*state).second->deviceState->name
		    << ", value=" << (*state).second->deviceState->value << ")");
	  if ((*state).second->allowedClasses) {
	    for (DbAllowedClassMap::iterator allowed = (*state).second->allowedClasses->begin();
		 allowed != (*state).second->allowedClasses->end(); ++allowed) {
	      if ((*allowed).second->mitigationDevice->tentativeBeamClass->number >
		  (*allowed).second->beamClass->number) {
		(*allowed).second->mitigationDevice->tentativeBeamClass =
		  (*allowed).second->beamClass;
		LOG_TRACE("ENGINE", (*allowed).second->mitigationDevice->name << " tentative class set to "
			  << (*allowed).second->beamClass->number);
	      }
	    }
	  }
	}
      }

      // If there are no faults, then enable the default - if there is one
      if ((*fault).second->defaultFaultState) {
	if ((*fault).second->defaultFaultState->faulted &&
	    !(*fault).second->defaultFaultState->ignored) {
	  LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value=" 
		    << (*fault).second->value << " (Default) fault state="
		    << (*fault).second->defaultFaultState->deviceState->name);
	  if ((*fault).second->defaultFaultState->allowedClasses) {
	    for (DbAllowedClassMap::iterator allowed = (*fault).second->defaultFaultState->allowedClasses->begin();
		 allowed != (*fault).second->defaultFaultState->allowedClasses->end(); ++allowed) {
	      if ((*allowed).second->mitigationDevice->tentativeBeamClass->number >
		  (*allowed).second->beamClass->number) {
		(*allowed).second->mitigationDevice->tentativeBeamClass =
		  (*allowed).second->beamClass;
		LOG_TRACE("ENGINE", (*allowed).second->mitigationDevice->name << " tentative class set to "
			  << (*allowed).second->beamClass->number);
	      }
	    }
	  }
	}
      }
    }
  }
}

int Engine::checkFaults() {
  if (!_mpsDb) {
    return 0;
  }

  if (_clearCheckFaultTime) {
    _checkFaultTime.clear();
    _clearCheckFaultTime = false;
  }

  // must first get updated input values
  _checkFaultTime.start();

  LOG_TRACE("ENGINE", "Checking faults");
  _mpsDb->lock();
  _mpsDb->clearMitigationBuffer();
  setTentativeBeamClass();
  evaluateFaults();
  bool reload = evaluateIgnoreConditions();
  mitigate();
  setAllowedBeamClass();
  _mpsDb->unlock();
  _checkFaultTime.end();

  // If FW configuration needs reloading, return non-zero value
  if (reload) {
    return 1;
  }
  return 0;
}

void Engine::showFaults() {
  if (!isInitialized()) {
    return;
  }

  // Print current faults
  bool faults = false;

  // Digital Faults
  _mpsDb->lock();
  for (DbFaultMap::iterator fault = _mpsDb->faults->begin();
       fault != _mpsDb->faults->end(); ++fault) {
    for (DbFaultStateMap::iterator state = (*fault).second->faultStates->begin();
	 state != (*fault).second->faultStates->end(); ++state) {
      if ((*state).second->faulted) {
	if (!faults) {
	  std::cout << "# Current faults:" << std::endl;
	  faults = true;
	}
	std::cout << "  " << (*fault).second->name << ": " << (*state).second->deviceState->name
		  << " (value=" << (*state).second->deviceState->value << ", ignored="
		  << (*state).second->ignored << ")" << std::endl;
      }
    }
  }
  _mpsDb->unlock();
  if (!faults) {
    std::cout << "# No faults" << std::endl;
  }
}

void Engine::showStats() {
  if (isInitialized()) {
    std::cout << ">> Engine Stats:" << std::endl;
    _checkFaultTime.show();
    _evaluationCycleTime.show();
    std::cout << "Rate: " << Engine::_rate << " Hz" << std::endl;
    std::cout << "Counter: " << Engine::_updateCounter << std::endl;
    std::cout << "Started at " << ctime(&Engine::_startTime) << std::endl;
    std::cout << &History::getInstance() << std::endl;
    _debugCounter = 0;
  }
  else {
    std::cout << "MPS not initialized - no database" << std::endl;
  }
}

void Engine::showFirmware() {
  std::cout << &(Firmware::getInstance());
}

void Engine::showMitigationDevices() {
  if (!isInitialized()) {
    std::cout << "MPS not initialized - no database" << std::endl;
    return;
  }

  std::cout << ">> Mitigation Devices: " << std::endl;
  _mpsDb->lock();
  for (DbMitigationDeviceMap::iterator mitigation = _mpsDb->mitigationDevices->begin();
       mitigation != _mpsDb->mitigationDevices->end(); ++mitigation) {
    std::cout << (*mitigation).second->name;
    if ((*mitigation).second->allowedBeamClass &&
	(*mitigation).second->tentativeBeamClass) {
      std::cout << ":\t Allowed " << (*mitigation).second->allowedBeamClass->number
		<< "/ Tentative " << (*mitigation).second->tentativeBeamClass->number
		<< std::endl;
    }
    else {
      std::cout << ":\t ERROR -> no beam class assigned (is MPS disabled?)" << std::endl;
    }
  }
  _mpsDb->unlock();
}

void Engine::showDeviceInputs() {
  if (!isInitialized()) {
    std::cout << "MPS not initialized - no database" << std::endl;
    return;
  }

  std::cout << "Device Inputs: " << std::endl;
  _mpsDb->lock();
  for (DbDeviceInputMap::iterator input = _mpsDb->deviceInputs->begin();
       input != _mpsDb->deviceInputs->end(); ++input) {
    std::cout << (*input).second << std::endl;
  }
  _mpsDb->unlock();
}

void Engine::showDatabaseInfo() {
  if (!isInitialized()) {
    std::cout << "MPS not initialized - no database" << std::endl;
    return;
  }
  _mpsDb->showInfo();
}

void Engine::startUpdateThread() {
  if (pthread_create(&_engineThread, 0, Engine::engineThread, 0)) {
    throw(EngineException("ERROR: Failed to start update thread"));
  }
}

uint32_t Engine::getUpdateRate() {
  return Engine::_rate;
}

uint32_t Engine::getUpdateCounter() {
  return Engine::_updateCounter;
}

time_t Engine::getStartTime() {
  return Engine::_startTime;
}

void Engine::threadExit() {
  _evaluate = false;
}

void Engine::threadJoin() {
  pthread_join(_engineThread, NULL);
}

#define MAX_SAFE_STACK (8*1024) /* The maximum stack size which is
                                   guaranteed safe to access without
                                   faulting */

void stack_prefault(void) {
    unsigned char dummy[MAX_SAFE_STACK];

    memset(dummy, 0, MAX_SAFE_STACK);
    return;
}

void *Engine::engineThread(void *arg) {
  struct sched_param  param;

  std::cout << "Engine: update thread started." << std::endl;
  // Declare as real time task
  param.sched_priority = 70;
  if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
    perror("Set priority");
    std::cerr << "WARN: Setting thread RT priority failed." << std::endl;
    //    exit(-1);
  }

  // Lock memory
  if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
    perror("mlockall failed");
    std::cerr << "WARN: mlockall failed." << std::endl;
  }

  // Pre-fault our stack
  stack_prefault();

  Engine::getInstance()._evaluationCycleTime.clear();

  Firmware::getInstance().setEnable(true);
  Firmware::getInstance().softwareClear();
  Firmware::getInstance().setSoftwareEnable(true);
  Firmware::getInstance().setTimingCheckEnable(true);

  time_t before = time(0);
  time_t now;
  _startTime = before;
  _updateCounter = 0;
  uint32_t counter = 0;
  bool reload = false;

  while(_evaluate) {
    pthread_mutex_lock(&_engineMutex);

    if (Engine::getInstance()._mpsDb) {
      if (Engine::getInstance()._mpsDb->updateInputs()) {
	reload = false;
	if (Engine::getInstance().checkFaults() > 0) {
	  reload = true;
	}
	Engine::getInstance()._mpsDb->mitigate(); // Write the mitigation to FW
	_updateCounter++;
	counter++;
	now = time(0);
	if (now > before) {
	  time_t diff = now - before; 
	  before = now;
	  _rate = counter / diff;
	  counter = 0;
	}
	Firmware::getInstance().heartbeat();
      }
      else {
	_rate = 0;
      }
      Engine::getInstance()._evaluationCycleTime.end();

      if (Engine::getInstance()._clearEvaluationCycleTime) {
	Engine::getInstance()._evaluationCycleTime.clear();
	Engine::getInstance()._clearEvaluationCycleTime = false;
      }

      // Reloads FW configuration - cause by ignore logic that 
      // enables/disables faults based on fast analog devices
      if (reload) {
	Engine::getInstance().reloadConfigFromIgnore();
      }
      Engine::getInstance()._mpsDb->_inputDelayTime.start();
    }
    else {
      _rate = 0;
      sleep(1);
    }

    pthread_mutex_unlock(&_engineMutex);
  }

  Firmware::getInstance().setSoftwareEnable(false);
  Firmware::getInstance().setEnable(false);

  std::cout << "EngineThread: Exiting..." << std::endl;
  pthread_exit(0);
}

void Engine::clearCheckTime() {
  _clearCheckFaultTime = true;
  _clearEvaluationCycleTime = true;
}

long Engine::getMaxCheckTime() {
  return _checkFaultTime.getMax();
}

long Engine::getAvgCheckTime() {
  return _checkFaultTime.getAverage();
}

long Engine::getMaxEvalTime() {
  return _evaluationCycleTime.getMax();
}

long Engine::getAvgEvalTime() {
  return _evaluationCycleTime.getAverage();
}
