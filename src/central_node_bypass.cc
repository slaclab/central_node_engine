#include <iostream>
#include <sstream>
#include <central_node_bypass_manager.h>
#include <central_node_bypass.h>
#include <central_node_exception.h>

/**
 * This creates bypasses for all digital/analog inputs. Should be invoked
 * when the engine loads its first configuration. The number and types of 
 * inputs must be the same for all configurations loaded. The bypass information
 * is shared by the loaded configs.
 */
void BypassManager::createBypassMap(MpsDbPtr db) {
  FaultInputBypassMap *map = new FaultInputBypassMap();
  bypassMap = FaultInputBypassMapPtr(map);

  int bypassId = 0;
  for (DbFaultInputMap::iterator input = db->faultInputs->begin();
       input != db->faultInputs->end(); ++input) {
    FaultInputBypass *bypass = new FaultInputBypass();
    bypass->id = bypassId;
    bypass->faultInputId = (*input).second->id;
    bypass->type = BYPASS_DIGITAL;
    bypass->validUntil = 0; // time is seconds since 1970, this gets set later
    // this field will get set later when bypasses start to be monitored
    bypass->valid = false;  

    bypassId++;
    
    FaultInputBypassPtr bypassPtr = FaultInputBypassPtr(bypass);
    bypassMap->insert(std::pair<int, FaultInputBypassPtr>(bypassId, bypassPtr));
  }

  for (DbAnalogDeviceMap::iterator analogInput = db->analogDevices->begin();
       analogInput != db->analogDevices->end(); ++analogInput) {
    FaultInputBypass *bypass = new FaultInputBypass();
    bypass->id = bypassId;
    bypass->faultInputId = (*analogInput).second->id;
    bypass->type = BYPASS_ANALOG;
    bypass->validUntil = 0; // time is seconds since 1970, this gets set later
    // this field will get set later when bypasses start to be monitored
    bypass->valid = false;  

    bypassId++;
    
    FaultInputBypassPtr bypassPtr = FaultInputBypassPtr(bypass);
    bypassMap->insert(std::pair<int, FaultInputBypassPtr>(bypassId, bypassPtr));
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

  for (FaultInputBypassMap::iterator bypass = bypassMap->begin();
       bypass != bypassMap->end(); ++bypass) {
    int inputId = (*bypass).second->faultInputId;
    if ((*bypass).second->type == BYPASS_DIGITAL) {
      DbFaultInputMap::iterator digitalInput = db->faultInputs->find(inputId);
      if (digitalInput == db->faultInputs->end()) {
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
	(*analogInput).second->bypass = (*bypass).second;
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
