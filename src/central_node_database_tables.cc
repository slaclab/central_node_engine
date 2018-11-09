#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/emit.h>

#include <central_node_database_tables.h>
#include <central_node_yaml.h>
#include <central_node_firmware.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <stdio.h>
#include <log_wrapper.h>

DbEntry::DbEntry() : id(-1) {};

std::ostream & operator<<(std::ostream &os, DbEntry * const entry) {
  os << " id[" << entry->id << "];";
  return os;
}

DbCrate::DbCrate() : DbEntry(), number(-1), numSlots(-1), shelfNumber(-1) {};

std::ostream & operator<<(std::ostream &os, DbCrate * const crate) {
  os << "id[" << crate->id << "]; "
     << "number[" << crate->number << "]; "
     << "slots[" << crate->numSlots << "]; "
     << "shelf[" << crate->shelfNumber << "]";
  return os;
}

DbInfo::DbInfo() {};

std::ostream & operator<<(std::ostream &os, DbInfo * const dbInfo) {
  os << "Source      : " << dbInfo->source << std::endl
     << "  Generated on: " << dbInfo->date << std::endl
     << "  User        : " << dbInfo->user << std::endl
     << "  Db md5sum   : " << dbInfo->md5sum << std::endl;
  return os;
}

DbApplicationType::DbApplicationType() : DbEntry(), number(-1),
					 analogChannelCount(-1), analogChannelSize(-1),
					 digitalChannelCount(-1), digitalChannelSize(-1),
					 description("empty") {
}

std::ostream & operator<<(std::ostream &os, DbApplicationType * const appType) {
  os << "id[" << appType->id << "]; "
     << "number[" << appType->number << "]; "
     << "analogChannelCount[" << appType->analogChannelCount << "]; "
     << "analogChannelSize[" << appType->analogChannelSize << "]; "
     << "digitalChannelCount[" << appType->digitalChannelCount << "]; "
     << "digitalChannelSize[" << appType->digitalChannelSize << "]; "
     << "description[" << appType->description << "]";
  return os;
}

DbChannel::DbChannel() : DbEntry(), number(-1), cardId(-1) {
}

std::ostream & operator<<(std::ostream &os, DbChannel * const channel) {
  os << channel->name << " : "
     << "card=" << channel->cardId << " : "
     << "channel=" << channel->number << " : "
     << "dbId=" << channel->id;
  /*
  os << "id[" << channel->id << "]; "
     << "number[" << channel->number << "]; "
     << "cardId[" << channel->cardId << "]; "
     << "name[" << channel->name << "]";
  */
  return os;
}

DbDeviceState::DbDeviceState() : DbEntry(), deviceTypeId(0), value(0), mask(0), name("") {
}

// Returns the integrator # based on the DeviceState value
// The integrator # is valid only for AnalogDevices, where the value
// is a single bit set within the 32 bits of the device value.
int DbDeviceState::getIntegrator() {
  if (value < 0x100) {
    return 0;
  }
  else if (value < 0x10000) {
    return 1;
  }
  else if (value < 0x1000000) {
    return 2;
  }
  else {
    return 3;
  }
}

std::ostream & operator<<(std::ostream &os, DbDeviceState * const devState) {
  os << "id[" << devState->id << "]; "
     << "deviceTypeId[" << devState->deviceTypeId << "]; "
     << "value[0x" << std::hex << devState->value << "]; "
     << "mask[0x" << devState->mask << std::dec << "]; "
     << "name[" << devState->name << "]";
  return os;
}

DbDeviceType::DbDeviceType() : DbEntry(), name("") {
}

std::ostream & operator<<(std::ostream &os, DbDeviceType * const devType) {
  os << "id[" << devType->id << "]; "
     << "name[" << devType->name << "]; "
     << "numIntegrators[" << devType->numIntegrators << "]";

  if (devType->deviceStates) {
    os << " deviceStates [";
    for (DbDeviceStateMap::iterator deviceState = devType->deviceStates->begin();
	 deviceState != devType->deviceStates->end(); ++deviceState) {
      os << (*deviceState).second->id << " ";
    }
    os << "]";
  }
  return os;
}

DbDeviceInput::DbDeviceInput() : DbEntry(),
				 bitPosition(-1), channelId(-1), faultValue(0),
				 digitalDeviceId(-1), value(0), previousValue(0),
				 latchedValue(0), invalidValueCount(0),
				 fastEvaluation(false) {
}

void DbDeviceInput::unlatch() {
  latchedValue = value;
}

std::ostream & operator<<(std::ostream &os, DbDeviceInput * const deviceInput) {
  os << "DeviceInput: "
     << "deviceId=" << deviceInput->digitalDeviceId << " : "
     << "channelId=" << deviceInput->channelId << " : "
     << "bitPos=" << deviceInput->bitPosition << " : "
     << "faultValue=" << deviceInput->faultValue << " : "
     << "value=" << deviceInput->value << " [wasLow=" << deviceInput->wasLowBit << ", wasHigh=" << deviceInput->wasHighBit << "]";
  if (deviceInput->fastEvaluation) {
    os << " [in fast device]";
  }
  if (deviceInput->bypass) {
    if (deviceInput->bypass->status == BYPASS_VALID) {
      os << " [Bypassed to " << deviceInput->bypass->value << "]";
    }
  }
  /*
  os << "id[" << deviceInput->id << "]; "
     << "bitPosition[" << deviceInput->bitPosition << "]; "
     << "faultValue[" << deviceInput->faultValue << "]; "
     << "channelId[" << deviceInput->channelId << "]; "
     << "digitalDeviceId[" << deviceInput->digitalDeviceId << "]; "
     << "value[" << deviceInput->value << ", wasLow=" << deviceInput->wasLowBit << ", wasHigh=" << deviceInput->wasHighBit << "]";
  if (deviceInput->fastEvaluation) {
    os << " [in fast device]";
  }
  if (deviceInput->bypass->status == BYPASS_VALID) {
    os << " [Bypassed to " << deviceInput->bypass->value << "]";
  }
  */
  return os;
}

DbDigitalDevice::DbDigitalDevice() : DbEntry(), deviceTypeId(-1) {
}

std::ostream & operator<<(std::ostream &os, DbDigitalDevice * const digitalDevice) {
  os << "DigitalDevice: "
     << digitalDevice->name << " : "
     << digitalDevice->description << " : ";

  if (digitalDevice->evaluation == FAST_EVALUATION) {
    os << "eval=fast : ";
  }
  else {
    os << "eval=slow : ";
  }

  os << "type=" << digitalDevice->deviceTypeId << " : "
     << "card=" << digitalDevice->cardId << " : "
     << "value=" << std::hex << digitalDevice->value << std::dec << " : "
     << "dbId=" << digitalDevice->id;

  os << std::endl;

  for (DbDeviceInputMap::iterator input = digitalDevice->inputDevices->begin();
       input != digitalDevice->inputDevices->end(); ++input) {
    os << " * " << (*input).second;
    os << std::endl;
  }

  os << " + States:" << std::endl;

  for (DbDeviceStateMap::iterator state = digitalDevice->deviceType->deviceStates->begin();
       state != digitalDevice->deviceType->deviceStates->end(); ++state) {
    os << "   - " << (*state).second << std::endl;
  }

  return os;
  /*
  os << "id[" << digitalDevice->id << "]; "
     << "deviceTypeId[" << digitalDevice->deviceTypeId << "]; "
     << "name[" << digitalDevice->name << "]; "
     << "evaluation[" << digitalDevice->evaluation << "]; "
     << "description[" << digitalDevice->description << "]; "
     << "cardId[" << digitalDevice->cardId << "] "
     << "value[" << digitalDevice->value << "]";
  return os;
  */
}

/**
 * Unlatch the bit set in the threshold mask passed as input parameter.
 *
 * @return current value for the bit specified by the mask
 */
uint32_t DbAnalogDevice::unlatch(uint32_t mask) {
  // Get the threshold bit from current value
  uint32_t currentBitValue = (value & mask);

  // Copy the current threshold bit value to the latchedValue
  latchedValue &= (currentBitValue & ~mask);

  return currentBitValue;
}

std::ostream & operator<<(std::ostream &os, DbAnalogDevice * const analogDevice) {
  os << "AnalogDevice: "
     << analogDevice->name << " : "
     << analogDevice->description << " : ";

  if (analogDevice->evaluation == FAST_EVALUATION) {
    os << "eval=fast : ";
  }
  else {
    os << "eval=slow : ";
  }

  os << "type=" << analogDevice->deviceTypeId << " : "
     << "card=" << analogDevice->cardId << " : "
     << "value=0x" << std::hex << analogDevice->value << std::dec << " : "
     << "dbId=" << analogDevice->id << " : " << std::endl;
  if (analogDevice->ignored) {
    os << "  ignored=YES";
  }
  else {
    os << "  ignored=no";
  }
  os << " [";
  for (int i = 0; i < ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL; ++i) {
    if (analogDevice->ignoredIntegrator[i]) {
      os << "I";
    }
    else {
      os << "-";
    }
  }
  os << "] : bypassMask=0x" << std::hex << analogDevice->bypassMask << std::dec << " : ";

  if (analogDevice->evaluation == FAST_EVALUATION) {
    os << "destinationMasks=";
    uint32_t integratorsPerChannel = analogDevice->deviceType->numIntegrators;

    for (uint32_t i = 0; i < integratorsPerChannel; ++i) {
      os << std::hex << analogDevice->fastDestinationMask[i];
      if (i < integratorsPerChannel - 1) {
	os << ", ";
      }
      os << std::dec;
    }
    os << std::endl;
    os << "  powerClasses=";
    for (uint32_t i = 0; i < integratorsPerChannel * ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
      os << analogDevice->fastPowerClass[i];
      if (i < (integratorsPerChannel * ANALOG_CHANNEL_INTEGRATORS_SIZE) - 1) {
	os << ", ";
      }
    }
  }

  os << std::endl;
  os << " * " << analogDevice->channel;

  return os;
  /*
  os << "id[" << analogDevice->id << "]; "
     << "deviceTypeId[" << analogDevice->deviceTypeId << "]; "
     << "name[" << analogDevice->name << "]; "
     << "description[" << analogDevice->description << "]; "
     << "evaluation[" << analogDevice->evaluation << "]; "
     << "channelId[" << analogDevice->channelId << "]; "
     << "cardId[" << analogDevice->cardId << "]; "
     << "ignored[" << analogDevice->ignored << "]; "
     << "bypassMask[0x" << std::hex << analogDevice->bypassMask << std::dec << "]";

  if (analogDevice->evaluation == FAST_EVALUATION) {
    os << "; destinationMasks[";
    uint32_t integratorsPerChannel = analogDevice->deviceType->numIntegrators;

    for (uint32_t i = 0; i < integratorsPerChannel; ++i) {
      os << std::hex << analogDevice->fastDestinationMask[i] << ", " << std::dec;
    }
    os << "]; powerClasses[";
    for (uint32_t i = 0; i < integratorsPerChannel * ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
      os << analogDevice->fastPowerClass[i] << ", ";
    }
    os << "]";
  }
  */
  return os;
}

DbApplicationCard::DbApplicationCard() {};

void DbApplicationCard::setUpdateBufferPtr(DataBuffer<uint8_t>* p)
{ 
    fwUpdateBuffer = p; 

    wasLowBufferOffset = 
      APPLICATION_UPDATE_BUFFER_HEADER_SIZE_BYTES +            // Skip header (timestamp + zeroes)
      globalId * APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES;  // Jump to correct area according to the globalId

    wasHighBufferOffset = 
        wasLowBufferOffset + 
        (APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES/2);        // Skip 192 bits from wasLow
}

ApplicationUpdateBufferBitSetHalf* DbApplicationCard::getWasLowBuffer()
{ 
    if (!fwUpdateBuffer)
        return NULL;

    return reinterpret_cast<ApplicationUpdateBufferBitSetHalf *>(&fwUpdateBuffer->getReadPtr()->at(wasLowBufferOffset));  
}

ApplicationUpdateBufferBitSetHalf* DbApplicationCard::getWasHighBuffer()
{
    if (!fwUpdateBuffer)
        return NULL;

    return reinterpret_cast<ApplicationUpdateBufferBitSetHalf *>(&fwUpdateBuffer->getReadPtr()->at(wasHighBufferOffset));
}

std::ostream & operator<<(std::ostream &os, DbApplicationCard * const appCard) {
  os << "App: " << appCard->name << " : "
     << appCard->description << std::endl
     << "  globalId=" << appCard->globalId << " : "
     << "dbId=" << appCard->id << " : "
     << "type=" << appCard->applicationTypeId << " : "
     << "number=" << appCard->number << " : "
     << "crateId=" << appCard->crateId << " : "
     << "slot=" << appCard->slotNumber << std::endl;

  if (appCard->digitalDevices) {
    os << "  DigitalDevices:" << std::endl;
    for (DbDigitalDeviceMap::iterator digitalDevice = appCard->digitalDevices->begin();
	 digitalDevice != appCard->digitalDevices->end(); ++digitalDevice) {
      os << " * " << (*digitalDevice).second->name << " [id=" << (*digitalDevice).second->id << "]" << std::endl;
    }
  }
  else if (appCard->analogDevices) {
    os << "  AnalogDevices:" << std::endl;
    for (DbAnalogDeviceMap::iterator analogDevice = appCard->analogDevices->begin();
	 analogDevice != appCard->analogDevices->end(); ++analogDevice) {
      os << " * " << (*analogDevice).second->name << " [id=" << (*analogDevice).second->id << "]" << std::endl;
    }
  }
  else {
    os << " - no devices (?)";
  }
  return os;
}

DbFaultInput::DbFaultInput() : DbEntry(), faultId(-1), deviceId(-1), bitPosition(-1) {
}

std::ostream & operator<<(std::ostream &os, DbFaultInput * const crate) {
  os << "id[" << crate->id << "]; "
     << "faultId[" << crate->faultId << "]; "
     << "deviceId[" << crate->deviceId << "]; "
     << "bitPosition[" << crate->bitPosition << "]";
  return os;
}

DbBeamClass::DbBeamClass() : DbEntry(), number(-1), name(""), description("") {
}

std::ostream & operator<<(std::ostream &os, DbBeamClass * const beamClass) {
  os << "id[" << beamClass->id << "]; "
     << "number[" << beamClass->number << "]; "
     << "name[" << beamClass->name << "] "
     << "integrationWindow[" << beamClass->integrationWindow << "]"
     << "minPeriod[" << beamClass->minPeriod << "]"
     << "totalCharge[" << beamClass->totalCharge << "]"
     << "description[" << beamClass->description << "]";
  return os;
}

DbBeamDestination::DbBeamDestination() : DbEntry(), name(""), softwareMitigationBufferIndex(0) {
}

std::ostream & operator<<(std::ostream &os, DbBeamDestination * const beamDestination) {
  os << "id[" << beamDestination->id << "]; "
     << "name[" << beamDestination->name << "]; "
     << "destinationMask[" << beamDestination->destinationMask << "]; "
     << "bufIndex[" << (int)beamDestination->softwareMitigationBufferIndex << "]; "
     << "shift[" << (int)beamDestination->bitShift << "]";
  if (beamDestination->allowedBeamClass) {
    os << "; Allowed[" << beamDestination->allowedBeamClass->number << "]";
  }
  if (beamDestination->tentativeBeamClass) {
    os << "; Tentative[" << beamDestination->tentativeBeamClass->number << "]";
  }
  if (beamDestination->previousAllowedBeamClass) {
    os << "; PrevAllowed["
       << beamDestination->previousAllowedBeamClass->number << "]";
  }
  return os;
}

DbAllowedClass::DbAllowedClass() : DbEntry(), beamClassId(-1), faultStateId(-1), beamDestinationId(-1) {
}

std::ostream & operator<<(std::ostream &os, DbAllowedClass * const allowedClass) {
  os << "id[" << allowedClass->id << "]; "
     << "beamClassId[" << allowedClass->beamClassId << "]; "
     << "faultStateId[" << allowedClass->faultStateId << "]; "
     << "beamDestinationId[" << allowedClass->beamDestinationId << "]";
  return os;
}

DbFaultState::DbFaultState() : DbEntry(), faultId(0), deviceStateId(0),
			       defaultState(false), faulted(true), ignored(false) {
}

std::ostream & operator<<(std::ostream &os, DbFaultState * const digitalFault) {
  os << "id[" << digitalFault->id << "]; "
     << "faultId[" << digitalFault->faultId << "]; "
     << "deviceStateId[" << digitalFault->deviceStateId << "]; "
     << "faulted[" << digitalFault->faulted << "]; "
     << "default[" << digitalFault->defaultState << "]";
  if (digitalFault->deviceState) {
    os << ": DeviceState[name=" << digitalFault->deviceState->name
       << ", value=" << digitalFault->deviceState->value << "]";
  }
  if (digitalFault->allowedClasses) {
    os << " : AllowedClasses[";
    unsigned int i = 1;
    for (DbAllowedClassMap::iterator it = digitalFault->allowedClasses->begin();
	 it != digitalFault->allowedClasses->end(); ++it, ++i) {
      os << (*it).second->beamDestination->name << "->"
	 << (*it).second->beamClass->name;
      if (i < digitalFault->allowedClasses->size()) {
	os << ", ";
      }
    }
    os << "]";
  }

  return os;
}

DbFault::DbFault() : DbEntry(), name(""), description(""), faulted(true), faultLatched(true), ignored(false), evaluation(SLOW_EVALUATION) {
}

void DbFault::unlatch() {
  faultLatched = faulted;
}

std::ostream & operator<<(std::ostream &os, DbFault * const fault) {
  os << "id[" << fault->id << "]; "
     << "name[" << fault->name << "]; "
     << "value[" << fault->value << "]; "
     << "faulted[" << fault->faulted << "]; "
     << "ignored[" << fault->ignored << "]; "
     << "evaluation[" << fault->evaluation << "]; "
     << "description[" << fault->description << "]";

  if (fault->faultInputs) {
    os << " : FaultInputs[";
    unsigned int i = 1;
    for (DbFaultInputMap::iterator it = fault->faultInputs->begin();
	 it != fault->faultInputs->end(); ++it, ++i) {
      if ((*it).second->digitalDevice) {
	os << (*it).second->digitalDevice->name;
	if (i < fault->faultInputs->size()) {
	  os << ", ";
	}
      }
      else {
	os << (*it).second->analogDevice->name;
	if (i < fault->faultInputs->size()) {
	  os << ", ";
	}
      }
    }
    os << "]";
  }

  if (fault->faultStates) {
    os << " : FaultStates[";
    unsigned int i = 1;
    for (DbFaultStateMap::iterator it = fault->faultStates->begin();
	 it != fault->faultStates->end(); ++it, ++i) {
      os << (*it).second->deviceState->name;
      if (i < fault->faultStates->size()) {
	os << ", ";
      }
    }
    os << "]";
  }

  if (fault->defaultFaultState) {
    os << " : Defaul["
       << fault->defaultFaultState->deviceState->name << "]";
  }

  return os;
}

DbConditionInput::DbConditionInput() : DbEntry(), bitPosition(0), faultStateId(0), conditionId(0) {
}

std::ostream & operator<<(std::ostream &os, DbConditionInput * const input) {
  os << "id[" << input->id << "]; "
     << "bitPosition[" << input->bitPosition << "]; "
     << "faultStateId[" << input->faultStateId << "]; "
     << "conditionId[" << input->conditionId << "]";
  return os;
}

DbIgnoreCondition::DbIgnoreCondition() : DbEntry(), conditionId(0), faultStateId(0), analogDeviceId(0) {
}

std::ostream & operator<<(std::ostream &os, DbIgnoreCondition * const input) {
  os << "id[" << input->id << "]; "
     << "faultStateId[" << input->faultStateId << "]; "
     << "conditionId[" << input->conditionId << "]";
  return os;
}

DbCondition::DbCondition() : DbEntry(), name(""), description(""), mask(0), state(false) {
}

std::ostream & operator<<(std::ostream &os, DbCondition * const fault) {
  os << "id[" << fault->id << "]; "
     << "name[" << fault->name << "]; "
     << "mask[0x" << fault->mask << std::dec << "]; "
     << "description[" << fault->description << "]";

  return os;
}
