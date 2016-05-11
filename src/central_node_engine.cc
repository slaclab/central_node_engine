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
    std::cout << "Device " << (*device).second->id << ": " << deviceValue << std::endl;
  }

  // Update fault values
  for (DbFaultMap::iterator fault = mpsDb->faults->begin();
       fault != mpsDb->faults->end(); ++fault) {
    int faultValue = 0;
    for (DbFaultInputMap::iterator input = (*fault).second->faultInputs->begin();
	 input != (*fault).second->faultInputs->end(); ++input) {
      //      int inputValue = mpsDb->digitalDevices->at((*input)->deviceId)->value;
      DbDigitalDeviceMap::iterator deviceIt = mpsDb->digitalDevices->find((*input).second->deviceId);
      if (deviceIt == mpsDb->digitalDevices->end()) {
	std::cerr << "ERROR updating faults" << std::endl;
	return -1;
      }
      int inputValue = (*deviceIt).second->value;
      faultValue |= (inputValue << (*input).second->bitPosition);
    }
    (*fault).second->update(faultValue);
    std::cout << "Fault " << (*fault).second->id << ": " << faultValue << std::endl;
  }

  return 0;
}

