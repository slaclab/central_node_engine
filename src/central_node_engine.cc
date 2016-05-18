#include <central_node_engine.h>

Engine::Engine() {
}

int Engine::loadConfig(std::string yamlFileName) {
  mpsDb = shared_ptr<MpsDb>(new MpsDb());

  if (mpsDb->load(yamlFileName) != 0) {
    std::cerr << "ERROR: Failed to load yaml database" << std::endl;
    return 1;
  }

  if (mpsDb->configure() != 0) {
    return 1;
  }

  return 0;
}

int Engine::updateInputs() {
  // Update values from firmware...
  return 0;
}

int Engine::checkFaults() {
  // Update digital devices
  for (DbDigitalDeviceMap::iterator device = mpsDb->digitalDevices->begin(); 
       device != mpsDb->digitalDevices->end(); ++device) {
    int deviceValue = 0;
    for (DbDeviceInputMap::iterator input = (*device).second->inputDevices->begin(); 
	 input != (*device).second->inputDevices->end(); ++input) {
      int inputValue = (*input).second->value;
      inputValue <<= (*input).second->bitPosition;
      deviceValue |= inputValue;
    }
    (*device).second->update(deviceValue);
  }

  // This finds the highest beam class - should be moved to initialization
  DbBeamClassPtr highestBeamClass;
  DbBeamClassPtr lowestBeamClass;
  int num = -1;
  int lowNum = 100;
  for (DbBeamClassMap::iterator beamClass = mpsDb->beamClasses->begin();
       beamClass != mpsDb->beamClasses->end(); ++beamClass) {
    if ((*beamClass).second->number > num) {
      highestBeamClass = (*beamClass).second;
    }
    if ((*beamClass).second->number < lowNum) {
      lowestBeamClass = (*beamClass).second;
    }
  }

  // Assigns highestBeamClass as tentativeBeamClass for all MitigationDevices
  for (DbMitigationDeviceMap::iterator device = mpsDb->mitigationDevices->begin();
       device != mpsDb->mitigationDevices->end(); ++device) {
    (*device).second->tentativeBeamClass = highestBeamClass;
    (*device).second->allowedBeamClass = lowestBeamClass;
  }

  // Update fault values and MitigationDevice allowed class
  for (DbFaultMap::iterator fault = mpsDb->faults->begin();
       fault != mpsDb->faults->end(); ++fault) {
    int faultValue = 0;
    for (DbFaultInputMap::iterator input = (*fault).second->faultInputs->begin();
	 input != (*fault).second->faultInputs->end(); ++input) {

      DbDigitalDeviceMap::iterator deviceIt = mpsDb->digitalDevices->find((*input).second->deviceId);
      if (deviceIt == mpsDb->digitalDevices->end()) {
	std::cerr << "ERROR updating faults" << std::endl;
	return -1;
      }
      int inputValue = (*deviceIt).second->value;
      faultValue |= (inputValue << (*input).second->bitPosition);
    }
    (*fault).second->update(faultValue);

    // Now that a Fault has a new value check the FaultStates
    for (DbDigitalFaultStateMap::iterator state = (*fault).second->digitalFaultStates->begin();
	 state != (*fault).second->digitalFaultStates->end(); ++state) {
      if ((*state).second->value == faultValue) {
	std::cout << "  *** FAULT " << (*fault).second->name << ": " 
		  << (*state).second->name << std::endl;
	(*state).second->faulted = true;
	if ((*state).second->allowedClasses) {
	  for (DbAllowedClassMap::iterator allowed = (*state).second->allowedClasses->begin();
	       allowed != (*state).second->allowedClasses->end(); ++allowed) {
	    // Update the allowedBeamClass for the MitigationDevices associated with this fault
	    if ((*allowed).second->mitigationDevice->tentativeBeamClass->number >
		(*allowed).second->beamClass->number) {
	      (*allowed).second->mitigationDevice->tentativeBeamClass =
		(*allowed).second->beamClass;
	    }
	  }
	}
      }
      else {
	if ((*state).second->faulted) {
	  std::cout << "  > Clearing fault " << (*fault).second->name << ": "
		    << (*state).second->name << std::endl;
	}
	(*state).second->faulted = false;
      }
    }
  }

  std::cout << "> Class at MitigationDevices: " << std::endl;
  for (DbMitigationDeviceMap::iterator it = mpsDb->mitigationDevices->begin(); 
       it != mpsDb->mitigationDevices->end(); ++it) {
    (*it).second->allowedBeamClass = (*it).second->tentativeBeamClass;
    std::cout << "  " <<  (*it).second->name << ": "
	      << (*it).second->allowedBeamClass->number << ", "
	      << (*it).second->allowedBeamClass->name << std::endl;
  }

  return 0;
}

