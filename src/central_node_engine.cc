#include <central_node_engine.h>

Engine::Engine() :
  checkFaultTime(5, "Evaluation time") {
}

int Engine::loadConfig(std::string yamlFileName) {
  std::stringstream errorStream;
  MpsDb *db = new MpsDb();
  mpsDb = shared_ptr<MpsDb>(db);

  if (mpsDb->load(yamlFileName) != 0) {
    errorStream << "ERROR: Failed to load yaml database ("
		<< yamlFileName << ")";
    throw(EngineException(errorStream.str()));
  }

  // When the first yaml configuration file is loaded the bypassMap
  // must be created.  
  if (!bypassManager) {
    bypassManager = BypassManagerPtr(new BypassManager());
    bypassManager->createBypassMap(mpsDb);
  }

  try {
    mpsDb->configure();
  } catch (DbException e) {
    delete db;
    throw e;
  }

  // Assign bypass to each digital/analog input
  bypassManager->assignBypass(mpsDb);

  // Find the lowest/highest BeamClasses - used when checking faults
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

  return 0;
}

int Engine::updateInputs() {
  // Update values from firmware...
  return 0;
}

int Engine::checkFaults() {
  checkFaultTime.start();

  // Update digital device values based on individual inputs
  for (DbDigitalDeviceMap::iterator device = mpsDb->digitalDevices->begin(); 
       device != mpsDb->digitalDevices->end(); ++device) {
    int deviceValue = 0;
    for (DbDeviceInputMap::iterator input = (*device).second->inputDevices->begin(); 
	 input != (*device).second->inputDevices->end(); ++input) {
      int inputValue = 0;
      if ((*input).second->bypass->status == BYPASS_VALID) {
	inputValue = (*input).second->bypass->value;
      }
      else {
	inputValue = (*input).second->value;
      }
      inputValue <<= (*input).second->bitPosition;
      deviceValue |= inputValue;
    }
    (*device).second->update(deviceValue);
  }
  
  // Assigns highestBeamClass as tentativeBeamClass for all MitigationDevices
  for (DbMitigationDeviceMap::iterator device = mpsDb->mitigationDevices->begin();
       device != mpsDb->mitigationDevices->end(); ++device) {
    (*device).second->tentativeBeamClass = highestBeamClass;
    (*device).second->allowedBeamClass = lowestBeamClass;
  }

  // Update digital Fault values and MitigationDevice allowed class
  for (DbFaultMap::iterator fault = mpsDb->faults->begin();
       fault != mpsDb->faults->end(); ++fault) {

    // First calculate the digital Fault value from the one or more inputs
    int faultValue = 0;
    for (DbFaultInputMap::iterator input = (*fault).second->faultInputs->begin();
	 input != (*fault).second->faultInputs->end(); ++input) {

      DbDigitalDeviceMap::iterator deviceIt = mpsDb->digitalDevices->find((*input).second->deviceId);
      if (deviceIt == mpsDb->digitalDevices->end()) {
	checkFaultTime.end();
	std::cerr << "ERROR updating faults" << std::endl;
	return -1;
      }
      int inputValue = (*deviceIt).second->value;
      faultValue |= (inputValue << (*input).second->bitPosition);
    }
    (*fault).second->update(faultValue);

    // Now that a Fault has a new value check if it is in the FaultStates list,
    // and update the allowedBeamClass for the MitigationDevices
    for (DbDigitalFaultStateMap::iterator state = (*fault).second->digitalFaultStates->begin();
	 state != (*fault).second->digitalFaultStates->end(); ++state) {
      if ((*state).second->value == faultValue) {
	(*state).second->faulted = true;
	if ((*state).second->allowedClasses) {
	  for (DbAllowedClassMap::iterator allowed = (*state).second->allowedClasses->begin();
	       allowed != (*state).second->allowedClasses->end(); ++allowed) {
	    // std::cout << "Mitigation: " << (*allowed).second->mitigationDevice->name
	    // 	      << ", Tentative [" << (*allowed).second->mitigationDevice->tentativeBeamClass->number
	    // 	      << "], Allowed [" << (*allowed).second->beamClass->number << "]"
	    // 	      << ", AllowedId [" << (*allowed).second->id << "]" << std::endl;
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
	  //	  std::cout << "  > Clearing fault " << (*fault).second->name << ": "
	  //		    << (*state).second->name << std::endl;
	}
	(*state).second->faulted = false;
      }
    }
  }

  // Update ThresholdFaults
  for (DbThresholdFaultStateMap::iterator faultState = mpsDb->thresholdFaultStates->begin();
       faultState != mpsDb->thresholdFaultStates->end(); ++faultState) {
    float deviceValue = (*faultState).second->thresholdFault->analogDevice->value;

    // Check if the device value exceeds threshold 
    if (deviceValue > (*faultState).second->thresholdFault->threshold) {
      (*faultState).second->faulted = true;
      // Update allowedClass for the ThresholdFaultState
      for (DbAllowedClassMap::iterator allowed = (*faultState).second->allowedClasses->begin();
	   allowed != (*faultState).second->allowedClasses->end(); ++allowed) {
	if ((*allowed).second->mitigationDevice->tentativeBeamClass->number >
	    (*allowed).second->beamClass->number) {
	  (*allowed).second->mitigationDevice->tentativeBeamClass =
	    (*allowed).second->beamClass;
	}
      }
    }
    else {
      (*faultState).second->faulted = false;
    }
  }

  //  std::cout << "> Class at MitigationDevices: " << std::endl;
  for (DbMitigationDeviceMap::iterator it = mpsDb->mitigationDevices->begin(); 
       it != mpsDb->mitigationDevices->end(); ++it) {
    // std::cout << "  " <<  (*it).second->name << ": "
    //   	      << (*it).second->allowedBeamClass->number << "/"
    // 	      << (*it).second->tentativeBeamClass->number << std::endl;
    (*it).second->allowedBeamClass = (*it).second->tentativeBeamClass;
  }

  checkFaultTime.end();

  return 0;
}

void Engine::showFaults() {
  // Print current faults
  bool faults = false;

  // Digital Faults
  for (DbFaultMap::iterator fault = mpsDb->faults->begin();
       fault != mpsDb->faults->end(); ++fault) {
    for (DbDigitalFaultStateMap::iterator state = (*fault).second->digitalFaultStates->begin();
	 state != (*fault).second->digitalFaultStates->end(); ++state) {
      if ((*state).second->faulted) {
	if (!faults) {
	  std::cout << "# Current digital faults:" << std::endl;
	  faults = true;
	}
	std::cout << "  " << (*fault).second->name << ": " << (*state).second->name << std::endl;
      }
    }
  }
  if (!faults) {
    std::cout << "# No digital faults" << std::endl;
  }
 
  // Analog Faults
  faults = false;
  for (DbThresholdFaultStateMap::iterator fault = mpsDb->thresholdFaultStates->begin();
       fault != mpsDb->thresholdFaultStates->end(); ++fault) {
    if((*fault).second->faulted) {
      if (!faults) {
	std::cout << "# Current analog faults:" << std::endl;
	faults = true;
      }
      std::cout << "  " << (*fault).second->thresholdFault->name << std::endl;
    }
  }
  if (!faults) {
    std::cout << "# No analog faults" << std::endl;
  }
}

void Engine::showStats() {
  std::cout << ">> Engine Stats:" << std::endl;
  checkFaultTime.show();
}

void Engine::showMitigationDevices() {
  std::cout << ">> Mitigation Devices: " << std::endl;
  for (DbMitigationDeviceMap::iterator mitigation = mpsDb->mitigationDevices->begin();
       mitigation != mpsDb->mitigationDevices->end(); ++mitigation) {
    std::cout << (*mitigation).second->name << ": "
	      << (*mitigation).second->allowedBeamClass->number << "/"
	      << (*mitigation).second->tentativeBeamClass->number << std::endl;
  }
}
