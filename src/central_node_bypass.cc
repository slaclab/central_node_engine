#include <iostream>
#include <sstream>
#include <stdio.h>
#include <time.h>

#include <central_node_bypass_manager.h>
#include <central_node_bypass.h>
#include <central_node_exception.h>
#include <central_node_history.h>
#include <central_node_engine.h>
#include <central_node_firmware.h>
#include <log_wrapper.h>

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
using namespace easyloggingpp;
static Logger *bypassLogger;
#endif

bool BypassManager::refreshFirmwareConfiguration = false;

BypassManager::BypassManager() :
  threadDone(false), _bypassThread(NULL), initialized(false) {
#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
  bypassLogger = Loggers::getLogger("BYPASS");
  LOG_TRACE("BYPASS", "Created BypassManager");
#endif
}

BypassManager::~BypassManager() {
}

bool BypassManager::isInitialized() {
  return initialized;
}

void BypassManager::startBypassThread() {
  if (_bypassThread != NULL) {
    _bypassThread->join();
  }

  _bypassThread = new std::thread(&BypassManager::bypassThread, this);

  if (pthread_setname_np(_bypassThread->native_handle(), "BypassThread")) {
    perror("pthread_setname_np failed");
  }
}

void BypassManager::stopBypassThread() {
  std::cout << "INFO: bypassThread stopping..." << std::endl;
  threadDone = true;
  if (_bypassThread != NULL) {
    _bypassThread->join();
  }
  std::cout << "INFO: bypassThread stopped..." << std::endl;
}

/**
 * This creates bypasses for all digital/analog inputs. Should be invoked
 * when the engine loads its first configuration. The number and types of
 * inputs must be the same for all configurations loaded. The bypass information
 * is shared by the loaded configs.
 */
void BypassManager::createBypassMap(MpsDbPtr db) {

  LOG_TRACE("BYPASS", "Creating bypass map");

  InputBypassMap *map = new InputBypassMap();
  bypassMap = InputBypassMapPtr(map);

  int bypassId = 0;
  for (DbFaultInputMap::iterator input = db->faultInputs->begin();
       input != db->faultInputs->end(); ++input) {
    InputBypass *bypass = new InputBypass();
    bypass->id = bypassId;
    bypass->deviceId = (*input).second->id;
    bypass->type = BYPASS_DIGITAL;
    bypass->until = 0; // time is seconds since 1970, this gets set later
    // this field will get set later when bypasses start to be monitored
    bypass->status = BYPASS_EXPIRED;
    bypass->index = 0;
    //    std::cout << (*input).second << std::endl;
    if ((*input).second->fastEvaluation) {
      bypass->configUpdate = true;
    }

    bypassId++;

    InputBypassPtr bypassPtr = InputBypassPtr(bypass);
    bypassMap->insert(std::pair<int, InputBypassPtr>(bypassId, bypassPtr));
  }

  for (DbAnalogChannelMap::iterator analogInput = db->analogChannels->begin();
       analogInput != db->analogChannels->end(); ++analogInput) {
    uint32_t integratorsPerChannel = ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL;
    /*
    if ((*analogInput).second->deviceType) {
      integratorsPerChannel = (*analogInput).second->deviceType->numIntegrators;
    }
    else {
      std::cout << "What?!?" << std::endl; exit(0);
    }
    */
    // Create one InputBypass for each integrator (max of 4 integrators)
    for (uint32_t i = 0; i < integratorsPerChannel; ++i) {
      InputBypass *bypass = new InputBypass();
      bypass->id = bypassId;
      bypass->deviceId = (*analogInput).second->id;
      bypass->type = BYPASS_ANALOG;
      bypass->until = 0; // time is seconds since 1970, this gets set later
      // this field will get set later when bypasses start to be monitored
      bypass->status = BYPASS_EXPIRED;
      bypass->index = i;
      if ((*analogInput).second->evaluation != 0) {
	      bypass->configUpdate = true;
      }

      bypassId++;

      InputBypassPtr bypassPtr = InputBypassPtr(bypass);
      bypassMap->insert(std::pair<int, InputBypassPtr>(bypassId, bypassPtr));
    }
  }
}

/**
 * After loading a configuration this must be called to set the bypass values to the
 * digital/analog inputs.
 *
 * Iterate over the bypassMap finding the proper inputs. All inputs must have bypasses,
 * if there are bypasses without inputs or inputs without bypasses an exception is
 * thrown.
 */
void BypassManager::assignBypass(MpsDbPtr db) {

  std::stringstream errorStream;
  LOG_TRACE("BYPASS", "Assigning bypass slots to MPS database inputs (analog/digital)");

  std::unique_lock<std::mutex> lock(mutex);

  for (InputBypassMap::iterator bypass = bypassMap->begin();
       bypass != bypassMap->end(); ++bypass) {
    int inputId = (*bypass).second->deviceId;
    if ((*bypass).second->type == BYPASS_DIGITAL) {
      DbFaultInputMap::iterator digitalInput = db->faultInputs->find(inputId);
      if (digitalInput == db->faultInputs->end()) {
	      errorStream << "ERROR: Failed to find FaultInput ("
		    << inputId << ") when assigning digital bypass";
	      throw(CentralNodeException(errorStream.str()));
      }
      else {
        (*digitalInput).second->bypass = (*bypass).second;
        // Make sure that changes to the bypass for this digital input causes firmware reload
        if ((*digitalInput).second->fastEvaluation) {
          (*digitalInput).second->bypass->configUpdate = true;
        }
      }
    }
    else { // BYPASS_ANALOG
      DbAnalogChannelMap::iterator analogInput = db->analogChannels->find(inputId);
      if (analogInput == db->analogChannels->end()) {
      	errorStream << "ERROR: Failed to find FaultInput ("
		    << inputId << ") when assigning analog bypass";
        throw(CentralNodeException(errorStream.str()));
      }
      else {
        (*analogInput).second->bypass[(*bypass).second->index] = (*bypass).second;
        (*bypass).second->bypassMask = &((*analogInput).second->bypassMask);
      }
    }
  }

  // Now makes sure all digital/analog inputs have a bypass assigned.
  errorStream << "ERROR: Failed to find bypass for FaultInputs [";
  bool error = false;
  bool first = true;
  for (DbFaultInputMap::iterator digitalInput = db->faultInputs->begin();
       digitalInput != db->faultInputs->end(); ++digitalInput) {
    if (!(*digitalInput).second->bypass) {
      if (!first) {
	      errorStream << ", ";
      }
      errorStream << (*digitalInput).second->id;
      error = true;
      first = false;
    }
  }
  errorStream << "]; AnalogChannels [";
  first = true;
  for (DbAnalogChannelMap::iterator analogInput = db->analogChannels->begin();
       analogInput != db->analogChannels->end(); ++analogInput) {
    if (!(*analogInput).second->bypass) {
      if (!first) {
	      errorStream << ", ";
      }
      errorStream << (*analogInput).second->id;
      error = true;
      first = false;
    }
  }
  errorStream << "]";

  if (error) {
    throw(CentralNodeException(errorStream.str()));
  }

  initialized = true;
}

/**
 * Check if there are expired bypasses in the priority queue.
 * If expired, the bypass is removed from the queue and
 * marked BYPASS_EXPIRED.
 *
 * @param testTime number of seconds since the Epoch (00:00:00 UTC, January 1, 1970)
 * used to check if bypasses are expired. The default value is zero,
 * meaning that the current time, returned by the time(time_t *t) function
 * call is used for comparison.
 */
void BypassManager::checkBypassQueue(time_t testTime) {
  time_t now;

  // Use specified test time if non-zero
  if (testTime == 0) {
    time(&now);
  }
  else {
    now = testTime;
  }

  //  LOG_TRACE("BYPASS", "Checking for expired bypasses (time=" << now
  //	    << ", size=" << bypassQueue.size() << ")");

  bool expired = true;
  {
    std::unique_lock<std::mutex> lock(mutex);
    while (expired) {
      expired = checkBypassQueueTop(now);
    }
  }
}

/**
 * Check if the top element in queue is expired. If so
 * remove it from the queue, mark as expired and return
 * true (expired). Otherwise return false (bypass still valid).
 *
 * @param now current time, in seconds
 */
bool BypassManager::checkBypassQueueTop(time_t now) {
  BypassQueueEntry top;
  if (!bypassQueue.empty()) {
    top = bypassQueue.top();
    // The time from the priority queue expired
    if (top.first <= now) {
      LOG_TRACE("BYPASS", "Bypass for device [" << top.second->deviceId << "] expired, "
		<< "type=" << top.second->type
		<< ", until=" << top.first << " sec"
		<< ", now=" << now << " sec"
		<< ", (actual until=" << top.second->until << ")");
      bypassQueue.pop();
      // Check if bypass expiration time (from the queue) is different
      // than top.first
      // - If top.second->until is greater than top.first, then the bypass
      //   was extended, i.e. there is at least one other entry in the priority
      //   queue for this bypass. In this case the bypass status is kept BYPASS_VALID
      // - If top.second->until is smaller than top.first then the bypass
      //   was shortened. In this case the bypass is set to BYPASS_EXPIRED
      // - If they are the same then the bypass is also set to BYPASS_EXPIRED
      if (top.second->until > top.first) {
	// If the bypass expiration time is greater than the current time then
	// the status must still be BYPASS_VALID
	if (top.second->status != BYPASS_VALID) {
	  LOG_TRACE("BYPASS", "Found BYPASS_EXPIRED status, setting back to BYPASS_VALID"
		    << " and returning error");
	  top.second->status = BYPASS_VALID;
	  return false;
	}
	else {
	  LOG_TRACE("BYPASS", "Found BYPASS_VALID, no bypass status change");
	}
      }
      else if (top.second->until <= top.first) {
	if (top.second->type == BYPASS_ANALOG) {
	  History::getInstance().logBypassState(top.second->deviceId,
						top.second->status,
						BYPASS_EXPIRED,
						top.second->index);
	  // If analog/threshold bypass, change bypassMask
	  // set integrator thresholds bit to 1 (not-bypassed)
	  uint32_t *bypassMask = top.second->bypassMask;//&((*analogInput).second->bypassMask);
	  if (top.second->index >= 0 && bypassMask != NULL) {
	    uint32_t m = 0xFF << (top.second->index * ANALOG_CHANNEL_INTEGRATORS_SIZE); // clear bypassed integrator thresholds
	    *bypassMask |= m;
	  }
	}
	else {
	  History::getInstance().logBypassState(top.second->deviceId,
						top.second->status,
						BYPASS_EXPIRED,
						BYPASS_DIGITAL_INDEX);
	}

	// Check if expired bypass requires firmware configuration update
	if (top.second->configUpdate) {
	  refreshFirmwareConfiguration = true;
	}

	top.second->status = BYPASS_EXPIRED;
	LOG_TRACE("BYPASS", "Setting status to BYPASS_EXPIRED");
      }
      return true;
    }
    else {
      return false;
    }
  }
  else {
    return false;
  }
}

/**
 * Add a new bypass to the bypassQueue. Possible scenarios are:
 *
 * 1) New bypass: the device has no bypass in the queue, add a new one
 * with the specified until time
 *
 * 2) Change bypass: there is already a bypass in the queue, the
 * new expiration time may be earlier or later than the previous.
 * In either case a new entry is added to the queue.
 * - If later: change the until field in the deviceBypass and keep
 *   the status BYPASS_VALID. When the first timer expires the
 *   checkBypassQueue verifies if the time from the queue is the
 *   same as the one saved in the bypass. The expiration time in
 *   the bypass is later, so the status is kept at BYPASS_VALID.
 *
 * - If earlier: change the until in the deviceBypass and
 *   keep the status set to BYPASS_VALID. The new timer will expire
 *   earlier, and when that happens the status should be changed
 *   to BYPASS_EXPIRED. When the later timer expires the status
 *   of the bypass will already be BYPASS_EXPIRED.
 *
 * 3) Cancel bypass: by specifying a bypassUntil parameter of ZERO. The
 *    until field in the bypass is set to ZERO and status changed to
 *    BYPASS_EXPIRED. No new element is added to the priority queue.
 *    If there are entries in the queue then they will be removed
 *    the next time the bypasses are checked.
 *
 * Multiple bypass to the same device can be added, the last one
 * to be added will be the the one that controls when the bypass
 * expires.
 */
void BypassManager::setBypass(MpsDbPtr db, BypassType bypassType,
			      uint32_t deviceId, uint32_t value, time_t bypassUntil,
			      bool test) {
  setThresholdBypass(db, bypassType, deviceId, value, bypassUntil, -1, test);
}

void BypassManager::setThresholdBypass(MpsDbPtr db, BypassType bypassType,
				       uint32_t deviceId, uint32_t value, time_t bypassUntil,
				       int intIndex, bool test) {

  InputBypassPtr bypass;
  std::stringstream errorStream;
  uint32_t *bypassMask = NULL;

  // Find the device and its bypass - all devices must have a bypass assigned.
  // The assignment must be after the BypassManager is created.
  if (bypassType == BYPASS_DIGITAL) {
    DbFaultInputMap::iterator digitalInput = db->faultInputs->find(deviceId);
    if (digitalInput == db->faultInputs->end()) {
      errorStream << "ERROR: Failed to find FaultInput[" << deviceId
		  << "] while setting bypass";
      throw(CentralNodeException(errorStream.str()));
    }
    bypass = (*digitalInput).second->bypass;
  }
  else {
    DbAnalogChannelMap::iterator analogInput = db->analogChannels->find(deviceId);
    if (analogInput == db->analogChannels->end()) {
      errorStream << "ERROR: Failed to find AnalogChannel[" << deviceId
		  << "] while setting bypass";
      throw(CentralNodeException(errorStream.str()));
    }
    bypass = (*analogInput).second->bypass[intIndex];
    bypassMask = &(*analogInput).second->bypassMask;
  }

  BypassQueueEntry newEntry;
  newEntry.first = bypassUntil;
  newEntry.second = bypass;

  // This handles case #3 - cancel bypass
  if (bypassUntil == 0) {
    if (bypass->type == BYPASS_ANALOG) {
      History::getInstance().logBypassState(bypass->deviceId,
					    bypass->status,
					    BYPASS_EXPIRED,
					    bypass->index);
    }
    else {
      History::getInstance().logBypassState(bypass->deviceId,
					    bypass->status,
					    BYPASS_EXPIRED,
					    BYPASS_DIGITAL_INDEX);
    }

    bypass->status = BYPASS_EXPIRED;
    bypass->until = 0;

    // If analog/threshold bypass, change bypassMask - set integrator thresholds bit to 1 (not-bypassed)
    if (intIndex >= 0 && bypassType == BYPASS_ANALOG && bypassMask != NULL) {
      uint32_t m = 0xFF << (intIndex * ANALOG_CHANNEL_INTEGRATORS_SIZE); // clear bypassed integrator thresholds
      *bypassMask |= m;
    }

    // Check if expired bypass requires firmware configuration update
    if (bypass->configUpdate) {
      refreshFirmwareConfiguration = true;
    }

    LOG_TRACE("BYPASS", "Set bypass EXPIRED for device [" << deviceId << "], "
	      << "type=" << bypassType);
  }
  else {
    time_t now;
    if (test) {
      now = bypassUntil - 1;
    }
    else {
      time(&now);
    }

    // This handles cases #1 and #2, for new and modified bypasses.
    if (bypassUntil > now) {
      if (bypass->type == BYPASS_ANALOG) {
	      History::getInstance().logBypassState(bypass->deviceId,
					      bypass->status,
					      BYPASS_VALID,
					      bypass->index);
      }
      else {
	      History::getInstance().logBypassState(bypass->deviceId,
					      bypass->status,
					      BYPASS_VALID,
					      BYPASS_DIGITAL_INDEX);
      }
      LOG_TRACE("BYPASS", "New bypass for device [" << deviceId << "], "
      << "type=" << bypassType << ", index=" << intIndex
      << ", until=" << bypassUntil << " sec"
      << ", now=" << now << " sec");
      // Check if expired bypass requires firmware configuration update
      if (bypass->configUpdate) {
	      refreshFirmwareConfiguration = true;
      }

      {
        std::unique_lock<std::mutex> lock(mutex);

        bypass->until = bypassUntil;
        bypass->status = BYPASS_VALID;
        bypass->value = value;

        // If analog/threshold bypass, change bypassMask - set threshold bit to 0 (bypassed)
        if (intIndex >= 0 && bypassType == BYPASS_ANALOG && bypassMask != NULL) {
          uint32_t m = ~(0xFF << (intIndex * ANALOG_CHANNEL_INTEGRATORS_SIZE)); // zero the bypassed threshold bit
          *bypassMask &= m;
        }

        bypassQueue.push(newEntry);
      }
    }
  }
}

void BypassManager::printBypassQueue() {
  if (!isInitialized()) {
    std::cout << "MPS not initialized - no database" << std::endl;
  }

  time_t now;
  time(&now);

  BypassPriorityQueue copy;
  {
    std::unique_lock<std::mutex> lock(mutex);
    copy = bypassQueue;
  }
  std::cout << "=== Bypass Queue (orded by expiration date) ===" << std::endl;
  std::cout << "=== Current time: " << now << "(s) ===" << std::endl;
  while (!copy.empty()) {
    struct tm *ptr;
    char buf[40];
    ptr = localtime(&copy.top().first);
    strftime(buf, 40, "%x %X", ptr);
    std::cout << buf << " (" << copy.top().first << "): deviceId=" << copy.top().second->deviceId;
    if (copy.top().second->type == BYPASS_ANALOG) {
      std::cout << " integrator " << copy.top().second->index;
    }
    if (copy.top().second->configUpdate) {
      std::cout << " [FW bypass]";
    }
    if (copy.top().second->status == BYPASS_VALID) {
      std::cout << " [VALID]";
    }
    else {
      std::cout << " [EXPIRED]";
    }
    std::cout << " BYPV=" << copy.top().second->value << std::endl;

    copy.pop();
  }
}

void BypassManager::bypassThread() {
  std::cout <<  "INFO: bypassThread started..." << std::endl;
  while(true) {
    if (threadDone) {
      LOG_TRACE("BYPASS", "Exiting bypassThread");
      std::cout << "INFO: bypassThread exiting..." << std::endl;
      return;
    }
    if (Engine::getInstance().getBypassManager()) {
      Engine::getInstance().getBypassManager()->checkBypassQueue();
      if (refreshFirmwareConfiguration) {
	Engine::getInstance().reloadConfig();
	refreshFirmwareConfiguration = false;
      }
      sleep(1);

      // Refresh the application timeout status - not really something related
      // to bypass, but it is done here for convenience
      Firmware::getInstance().getAppTimeoutStatus();
    }
  }
}

