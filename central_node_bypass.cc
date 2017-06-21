#include <iostream>
#include <sstream>
#include <stdio.h>
#include <time.h>

#include <central_node_bypass_manager.h>
#include <central_node_bypass.h>
#include <central_node_exception.h>
#include <log_wrapper.h>

#ifdef LOG_ENABLED
using namespace easyloggingpp;
static Logger *bypassLogger;
#endif

BypassManager::BypassManager() {
#ifdef LOG_ENABLED
  bypassLogger = Loggers::getLogger("BYPASS");
  LOG_TRACE("BYPASS", "Created BypassManager");
#endif
  int ret = pthread_mutex_init(&mutex, NULL);
  if (0 != ret) {
    throw("ERROR: BypassManager::BypassManager() failed to init mutex.");
  }
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
  for (DbDeviceInputMap::iterator input = db->deviceInputs->begin();
       input != db->deviceInputs->end(); ++input) {
    InputBypass *bypass = new InputBypass();
    bypass->id = bypassId;
    bypass->deviceId = (*input).second->id;
    bypass->type = BYPASS_DIGITAL;
    bypass->until = 0; // time is seconds since 1970, this gets set later
    // this field will get set later when bypasses start to be monitored
    bypass->status = BYPASS_EXPIRED;
    bypass->index = 0;

    bypassId++;
    
    InputBypassPtr bypassPtr = InputBypassPtr(bypass);
    bypassMap->insert(std::pair<int, InputBypassPtr>(bypassId, bypassPtr));
  }

  for (DbAnalogDeviceMap::iterator analogInput = db->analogDevices->begin();
       analogInput != db->analogDevices->end(); ++analogInput) {
    // Create one InputBypass for each threshold bit (max of 32 threshold bits)
    for (int i = 0; i < ANALOG_DEVICE_NUM_THRESHOLDS; ++i) {
      InputBypass *bypass = new InputBypass();
      bypass->id = bypassId;
      bypass->deviceId = (*analogInput).second->id;
      bypass->type = BYPASS_ANALOG;
      bypass->until = 0; // time is seconds since 1970, this gets set later
      // this field will get set later when bypasses start to be monitored
      bypass->status = BYPASS_EXPIRED;
      bypass->index = i;

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

  for (InputBypassMap::iterator bypass = bypassMap->begin();
       bypass != bypassMap->end(); ++bypass) {
    int inputId = (*bypass).second->deviceId;
    if ((*bypass).second->type == BYPASS_DIGITAL) {
      DbDeviceInputMap::iterator digitalInput = db->deviceInputs->find(inputId);
      if (digitalInput == db->deviceInputs->end()) {
	errorStream << "ERROR: Failed to find FaultInput ("
		    << inputId << ") when assigning bypass";
	throw(CentralNodeException(errorStream.str()));
      }
      else {
	(*digitalInput).second->bypass = (*bypass).second; 
      }
    }
    else { // BYPASS_ANALOG
      DbAnalogDeviceMap::iterator analogInput = db->analogDevices->find(inputId);
      if (analogInput == db->analogDevices->end()) {
	errorStream << "ERROR: Failed to find FaultInput ("
		    << inputId << ") when assigning bypass";
	throw(CentralNodeException(errorStream.str()));
      }
      else {
	(*analogInput).second->bypass[(*bypass).second->index] = (*bypass).second;
      }
    }
  }

  // Now makes sure all digital/analog inputs have a bypass assigned.
  errorStream << "ERROR: Failed to find bypass for FaultInputs [";
  bool error = false;
  bool first = true;
  for (DbDeviceInputMap::iterator digitalInput = db->deviceInputs->begin();
       digitalInput != db->deviceInputs->end(); ++digitalInput) {
    if (!(*digitalInput).second->bypass) {
      if (!first) {
	errorStream << ", ";
      }
      errorStream << (*digitalInput).second->id;
      error = true;
      first = false;
    }
  }
  errorStream << "]; AnalogDevices [";
  first = true;
  for (DbAnalogDeviceMap::iterator analogInput = db->analogDevices->begin();
       analogInput != db->analogDevices->end(); ++analogInput) {
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

  LOG_TRACE("BYPASS", "Checking for expired bypasses (time=" << now
	    << ", size=" << bypassQueue.size() << ")");

  bool expired = true;
  int ret = pthread_mutex_lock(&mutex);
  if (0 != ret) {
    throw("ERROR: BypassManager::checkBypassQueue() failed to lock mutex.");
  }

  while (expired) {
    expired = checkBypassQueueTop(now);
  }
  ret = pthread_mutex_unlock(&mutex);
  if (0 != ret) {
    throw("ERROR: BypassManager::checkBypassQueue() failed to unlock mutex.");
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
				       int thresholdIndex, bool test) {
  InputBypassPtr bypass;
  std::stringstream errorStream;
  uint32_t *bypassMask = NULL;

  // Find the device and its bypass - all devices must have a bypass assigned.
  // The assignment must be after the BypassManager is created.
  if (bypassType == BYPASS_DIGITAL) {
    DbDeviceInputMap::iterator digitalInput = db->deviceInputs->find(deviceId);
    if (digitalInput == db->deviceInputs->end()) {
      errorStream << "ERROR: Failed to find DeviceInput[" << deviceId
		  << "] while setting bypass";
      throw(CentralNodeException(errorStream.str()));
    }
    bypass = (*digitalInput).second->bypass;
  }
  else {
    DbAnalogDeviceMap::iterator analogInput = db->analogDevices->find(deviceId);
    if (analogInput == db->analogDevices->end()) {
      errorStream << "ERROR: Failed to find AnalogDevice[" << deviceId
		  << "] while setting bypass";
      throw(CentralNodeException(errorStream.str()));
    }
    bypass = (*analogInput).second->bypass[thresholdIndex];
    bypassMask = &(*analogInput).second->bypassMask;
  }

  BypassQueueEntry newEntry;
  newEntry.first = bypassUntil;
  newEntry.second = bypass;

  // This handles case #3 - cancel bypass  
  if (bypassUntil == 0) {
    bypass->status = BYPASS_EXPIRED;
    bypass->until = 0;

    // If analog/threshold bypass, change bypassMask - set threshold bit to 1 (not-bypassed)
    if (thresholdIndex >= 0 && bypassType == BYPASS_ANALOG && bypassMask != NULL) {
      uint32_t m = 1 << thresholdIndex; // clear bypassed threshold
      *bypassMask |= m;
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
      LOG_TRACE("BYPASS", "New bypass for device [" << deviceId << "], "
		<< "type=" << bypassType
		<< ", until=" << bypassUntil << " sec"
		<< ", now=" << now << " sec");
      int ret = pthread_mutex_lock(&mutex);
      if (0 != ret) {
	throw("ERROR: BypassManager::setBypass() failed to lock mutex.");
      }
      bypass->until = bypassUntil;
      bypass->status = BYPASS_VALID;
      bypass->value = value;

      // If analog/threshold bypass, change bypassMask - set threshold bit to 0 (bypassed)
      if (thresholdIndex >= 0 && bypassType == BYPASS_ANALOG && bypassMask != NULL) {
	uint32_t m = ~(1 << thresholdIndex); // zero the bypassed threshold bit
	*bypassMask &= m;
      }

      bypassQueue.push(newEntry);
      ret = pthread_mutex_unlock(&mutex);
      if (0 != ret) {
	throw("ERROR: BypassManager::setBypass() failed to unlock mutex.");
      }
    }
  }
}

void BypassManager::printBypassQueue() {
  int ret = pthread_mutex_lock(&mutex);
  if (0 != ret) {
    throw("ERROR: BypassManager::printBypassQueue() failed to lock mutex.");
  }
  BypassPriorityQueue copy = bypassQueue;
  ret = pthread_mutex_unlock(&mutex);
  if (0 != ret) {
    throw("ERROR: BypassManager::printBypassQueue() failed to unlock mutex.");
  }
  std::cout << "=== Bypass Queue (orded by expiration date) ===" << std::endl;
  while (!copy.empty()) {
    struct tm *ptr;
    char buf[40];
    ptr = localtime(&copy.top().first);
    strftime(buf, 40, "%x %X", ptr);
    std::cout << buf << " (" << copy.top().first << "): " << copy.top().second->deviceId;
    if (copy.top().second->status == BYPASS_VALID) {
      std::cout << " [VALID]" << std::endl;
    }
    else {
      std::cout << " [EXPIRED]" << std::endl;
    }
    copy.pop();
  }
}
