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

DbEntry::DbEntry() : id(999) {};

std::ostream & operator<<(std::ostream &os, DbEntry * const entry) {
  os << " id[" << entry->id << "];";
  return os;
}

DbCrate::DbCrate() : DbEntry(), crateId(999), numSlots(999), elevation(999) {};

std::ostream & operator<<(std::ostream &os, DbCrate * const crate) {
  os << "id[" << crate->id << "]; "
     << "crateId[" << crate->crateId << "]; "
     << "slots[" << crate->numSlots << "]; "
     << "location[" << crate->location << "]; "
     << "rack[" << crate->rack << "]; "
     << "elevation[" << crate->elevation << "]; "
     << "area[" << crate->area << "]; "
     << "node[" << crate->node << "]";
  return os;
}

DbLinkNode::DbLinkNode() : DbEntry(), rxPgp(999), lnType(999), lnId(999), crateId(999), groupId(999) {};

std::ostream & operator<<(std::ostream &os, DbLinkNode * const linkNode) {
  os << "id[" << linkNode->id << "]; "
     << "location[" << linkNode->location << "]; "
     << "group_link[" << linkNode->groupLink << "]; "
     << "rx_pgp[" << linkNode->rxPgp << "]; "
     << "ln_type[" << linkNode->lnType << "]; "
     << "crateId[" << linkNode->crateId << "]; "
     << "groupId[" << linkNode->groupId << "]";
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

DbApplicationType::DbApplicationType() : DbEntry(), num_integrators(999),
					 analogChannelCount(0), digitalChannelCount(0), softwareChannelCount(0),
					 name("empty") {
}

std::ostream & operator<<(std::ostream &os, DbApplicationType * const appType) {
  os << "id[" << appType->id << "]; "
     << "num_integrators[" << appType->num_integrators << "]; "
     << "analogChannelCount[" << appType->analogChannelCount << "]; "
     << "digitalChannelCount[" << appType->digitalChannelCount << "]; "
     << "softwareChannelCount[" << appType->softwareChannelCount << "]; "
     << "name[" << appType->name << "]";
  return os;
}

DbDigitalChannel::DbDigitalChannel() : DbEntry(), number(999), cardId(999) {
}

void DbDigitalChannel::unlatch() {
  latchedValue = value;
}

std::ostream & operator<<(std::ostream &os, DbDigitalChannel * const digitalChannel) {
  os << "id[" << digitalChannel->id << "]; "
     << "name[" << digitalChannel->name << "]; "
     << "cardId[" << digitalChannel->cardId << "]; "
     << "number[" << digitalChannel->number << "]; "
     << "z_name[" << digitalChannel->z_name << "]; "
     << "o_name[" << digitalChannel->o_name << "]; "
     << "monitored_pv[" << digitalChannel->monitored_pv << "]; "
     << "debounce[" << digitalChannel->debounce << "]; "
     << "alarm_state[" << digitalChannel->alarm_state << "]; "
     << "z_location[" << digitalChannel->z_location << "]; "
     << "auto_reset[" << digitalChannel->auto_reset << "]; ";

  if (digitalChannel->evaluation == FAST_EVALUATION) {
    os << "eval=fast : ";
  }
  else {
    os << "eval=slow : ";
  }

  os << "value=" << std::hex << digitalChannel->value << std::dec << " : "
     << "modeActive=" << digitalChannel->modeActive;

  if (digitalChannel->ignored) {
    os << " : ignored=YES";
  }
  else {
    os << " : ignored=no";
  }
  os << std::endl;

// TODO - Implement the new fault_inputs and fault_States here

  for (DbFaultInputMap::iterator faultInput = digitalChannel->inputFaults->begin();
       faultInput != digitalChannel->inputFaults->end(); ++faultInput) {
    os << " * " << (*faultInput).second;
    os << std::endl;
  }

  // os << " + States:" << std::endl;

  // for (DbDeviceStateMap::iterator state = digitalChannel->deviceType->deviceStates->begin();
  //      state != digitalChannel->deviceType->deviceStates->end(); ++state) {
  //   os << "   - " << (*state).second << std::endl;
  // }


  return os;
}

// TODO - Delete deviceInput once replaced with digitalChannel
DbDeviceInput::DbDeviceInput() : DbEntry(),
				 bitPosition(999), channelId(999), faultValue(0),
				 digitalDeviceId(999), value(0), previousValue(0),
				 latchedValue(0), invalidValueCount(0),
				 fastEvaluation(false), autoReset(0) {
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
     << "value=" << deviceInput->value << " [wasLow=" << deviceInput->wasLowBit << ", wasHigh=" << deviceInput->wasHighBit << "]" << " : "
     << "latchedValue=" << deviceInput->latchedValue;
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

DbDeviceState::DbDeviceState() : DbEntry(), deviceTypeId(0), value(0), mask(0), name("") {
}

// Returns the integrator # based on the DeviceState value
// The integrator # is valid only for AnalogChannels, where the value
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

/**
 * Unlatch the bit set in the threshold mask passed as input parameter.
 *
 * @return current value for the bit specified by the mask
 */
uint32_t DbAnalogChannel::unlatch(uint32_t mask) {
  // Get the threshold bit from current value
  uint32_t currentBitValue = (value & mask);

  // Copy the current threshold bit value to the latchedValue
  latchedValue &= (currentBitValue & ~mask);

  return currentBitValue;
}

// No DbAnalogChannel() declaration here because it exists in central_node_inputs.cc

std::ostream & operator<<(std::ostream &os, DbAnalogChannel * const analogChannel) {
  os << "id[" << analogChannel->id << "]; "
     << "name[" << analogChannel->name << "]; "
     << "cardId[" << analogChannel->cardId << "]; "
     << "number[" << analogChannel->number << "]; "
     << "offset[" << analogChannel->offset << "]; "
     << "slope[" << analogChannel->slope << "]; "
     << "egu[" << analogChannel->egu << "]; "
     << "integrator[" << analogChannel->integrator << "]; "
     << "gain_bay[" << analogChannel->gain_bay << "]; "
     << "gain_channel[" << analogChannel->gain_channel << "]; "
     << "z_location[" << analogChannel->z_location << "]; "
     << "auto_reset[" << analogChannel->auto_reset << "]; "
     << "evaluation[" << analogChannel->evaluation << "];";

  if (analogChannel->evaluation == FAST_EVALUATION) {
    os << "eval=fast : ";
  }
  else {
    os << "eval=slow : ";
  }

  os << "value=0x" << std::hex << analogChannel->value << std::dec << " : "
     << "latchedValue=0x" << std::hex << analogChannel->latchedValue << std::dec << " : "
     << "modeActive=" << analogChannel->modeActive << " : " << std::endl;
  if (analogChannel->ignored) {
    os << "  ignored=YES";
  }
  else {
    os << "  ignored=no";
  }
  os << " [";
  for (uint32_t i = 0; i < ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL; ++i) {
    if (analogChannel->ignoredIntegrator[i]) {
      os << "I";
    }
    else {
      os << "-";
    }
  }
  os << "] : bypassMask=0x" << std::hex << analogChannel->bypassMask << std::dec << " : ";

  if (analogChannel->evaluation == FAST_EVALUATION) {
    os << "destinationMasks=";
    uint32_t integratorsPerChannel = analogChannel->appType->num_integrators;

    for (uint32_t i = 0; i < integratorsPerChannel; ++i) {
      os << std::hex << analogChannel->fastDestinationMask[i];
      if (i < integratorsPerChannel - 1) {
	      os << ", ";
      }
      os << std::dec;
    }
    os << std::endl;
    os << "  powerClasses=";
    for (uint32_t i = 0; i < integratorsPerChannel * ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
      os << analogChannel->fastPowerClass[i];
      if (i < (integratorsPerChannel * ANALOG_CHANNEL_INTEGRATORS_SIZE) - 1) {
	      os << ", ";
      }
    }
  }

  return os;
}

DbApplicationCard::DbApplicationCard() {};

void DbApplicationCard::setUpdateBufferPtr(std::vector<uint8_t>* p)
{
    fwUpdateBuffer = p;

    wasLowBufferOffset =
      APPLICATION_UPDATE_BUFFER_HEADER_SIZE_BYTES +            // Skip header (timestamp + zeroes)
      number * APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES;  // Jump to correct area according to the applicationCard->number

    wasHighBufferOffset =
        wasLowBufferOffset +
        (APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES/2);        // Skip 192 bits from wasLow
}

ApplicationUpdateBufferBitSetHalf* DbApplicationCard::getWasLowBuffer()
{
    if (!fwUpdateBuffer)
        return NULL;

    return reinterpret_cast<ApplicationUpdateBufferBitSetHalf *>(&fwUpdateBuffer->at(wasLowBufferOffset));
}

ApplicationUpdateBufferBitSetHalf* DbApplicationCard::getWasHighBuffer()
{
    if (!fwUpdateBuffer)
        return NULL;

    return reinterpret_cast<ApplicationUpdateBufferBitSetHalf *>(&fwUpdateBuffer->at(wasHighBufferOffset));
}

std::ostream & operator<<(std::ostream &os, DbApplicationCard * const appCard) {
  os << "App: " << appCard->applicationType->name << std::endl
     << "  id[" << appCard->id << "]; "
     << "applicationTypeId[" << appCard->applicationTypeId << "]; "
     << "crateId[" << appCard->crateId << "]; "
     << "slotNumber[" << appCard->slotNumber << "]; "
     << "online[" << appCard->online << "]; "
     << "active[" << appCard->active << "]; "
     << "modeActive[" << appCard->modeActive << "]; "
     << "hasInputs[" << appCard->hasInputs << "]" << std::endl;

  if (appCard->digitalChannels) {
    os << "  DigitalChannels:" << std::endl;
    for (DbDigitalChannelMap::iterator digitalChannel = appCard->digitalChannels->begin();
	 digitalChannel != appCard->digitalChannels->end(); ++digitalChannel) {
      os << " * " << (*digitalChannel).second->name << " [id=" << (*digitalChannel).second->id << "]" << std::endl;
    }
  }
  else if (appCard->analogChannels) {
    os << "  AnalogChannels:" << std::endl;
    for (DbAnalogChannelMap::iterator analogChannel = appCard->analogChannels->begin();
	 analogChannel != appCard->analogChannels->end(); ++analogChannel) {
      os << " * " << (*analogChannel).second->name << " [id=" << (*analogChannel).second->id << "]" << std::endl;
    }
  }
  else {
    os << " - no devices (?)" << std::endl;
  }
  return os;
}

DbFaultInput::DbFaultInput() : DbEntry(), faultId(999), channelId(999), bitPosition(999) {
}

std::ostream & operator<<(std::ostream &os, DbFaultInput * const crate) {
  os << "id[" << crate->id << "]; "
     << "faultId[" << crate->faultId << "]; "
     << "channelId[" << crate->channelId << "]; "
     << "bitPosition[" << crate->bitPosition << "]";
  return os;
}

DbBeamClass::DbBeamClass() : DbEntry(), number(999), name("") {
}

std::ostream & operator<<(std::ostream &os, DbBeamClass * const beamClass) {
  os << "id[" << beamClass->id << "]; "
     << "number[" << beamClass->number << "]; "
     << "name[" << beamClass->name << "]; "
     << "integrationWindow[" << beamClass->integrationWindow << "]; "
     << "minPeriod[" << beamClass->minPeriod << "]; "
     << "totalCharge[" << beamClass->totalCharge << "]";
  return os;
}

DbBeamDestination::DbBeamDestination() : DbEntry(), name("") {
}

std::ostream & operator<<(std::ostream &os, DbBeamDestination * const beamDestination) {
  os << "id[" << beamDestination->id << "]; "
     << "name[" << beamDestination->name << "]; "
     << "destinationMask[" << beamDestination->destinationMask << "]; "
     << "displayOrder[" << beamDestination->displayOrder << "]";
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

DbAllowedClass::DbAllowedClass() : DbEntry(), beamClassId(999), beamDestinationId(999) {
}

std::ostream & operator<<(std::ostream &os, DbAllowedClass * const allowedClass) {
  os << "id[" << allowedClass->id << "]; "
     << "beamClassId[" << allowedClass->beamClassId << "]; "
     << "beamDestinationId[" << allowedClass->beamDestinationId << "]";
  return os;
}

DbFaultState::DbFaultState() : DbEntry(), faultId(0), mask(0),
			       defaultState(false), faulted(true), ignored(false) {
}

std::ostream & operator<<(std::ostream &os, DbFaultState * const digitalFault) {
  os << "id[" << digitalFault->id << "]; "
     << "faultId[" << digitalFault->faultId << "]; "
     << "mask[" << digitalFault->mask << "]; "
     << "name[" << digitalFault->name << "]; "
     << "faulted[" << digitalFault->faulted << "]; "
     << "default[" << digitalFault->defaultState << "]; "
     << "value[" << digitalFault->value << "]";

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

  // May omit this once these Ids are used to configure the important information
  if (!digitalFault->mitigationIds.empty()) { 
    os << " : MitigationIds[";
    unsigned int i = 0;
    for (; i < digitalFault->mitigationIds.size() - 1; i++) {
      os << digitalFault->mitigationIds.at(i) << ", ";
    }
    os << digitalFault->mitigationIds.at(i) << "]";
  }

  return os;
}

DbFault::DbFault() : DbEntry(), name(""), pv(""), faulted(true), ignored(false), evaluation(SLOW_EVALUATION), value(0) {
}

std::ostream & operator<<(std::ostream &os, DbFault * const fault) {
  os << "id[" << fault->id << "]; "
     << "name[" << fault->name << "]; "
     << "value[" << fault->value << "]; "
     << "faulted[" << fault->faulted << "]; "
     << "ignored[" << fault->ignored << "]; "
     << "evaluation[" << fault->evaluation << "]; "
     << "pv[" << fault->pv << "]; ";

  if (fault->faultInputs) {
    os << " : FaultInputs[";
    unsigned int i = 1;
    for (DbFaultInputMap::iterator it = fault->faultInputs->begin();
	 it != fault->faultInputs->end(); ++it, ++i) {
      if ((*it).second->digitalChannel) {
	os << (*it).second->digitalChannel->name;
	if (i < fault->faultInputs->size()) {
	  os << ", ";
	}
      }
      else {
	os << (*it).second->analogChannel->name;
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
        os << (*it).second->name;
        if (i < fault->faultStates->size()) {
            os << ", ";
        }
    }
    os << "]";
  }

  if (fault->defaultFaultState) {
    os << " : Default["
       << fault->defaultFaultState->name << "]";
  }

// May omit this once these Ids are used to configure the important information
  if (!fault->ignoreConditionIds.empty()) { 
    os << " : IgnoreConditionIds[";
    unsigned int i = 0;
    for (; i < fault->ignoreConditionIds.size() - 1; i++) {
      os << fault->ignoreConditionIds.at(i) << ", ";
    }
    os << fault->ignoreConditionIds.at(i) << "]";
  }

  return os;
}

DbIgnoreCondition::DbIgnoreCondition() : DbEntry(), name(""), description(""), value(0), digitalChannelId(0) {
}

std::ostream & operator<<(std::ostream &os, DbIgnoreCondition * const ignoreCondition) {
  os << "id[" << ignoreCondition->id << "]; "
     << "name[" << ignoreCondition->name << "]; "
     << "description[" << ignoreCondition->description << "]; "
     << "value[" << ignoreCondition->value << "]; "
     << "digitalChannelId[" << ignoreCondition->digitalChannelId << "]";


  // May omit this once these Ids are used to configure the important information
  if (ignoreCondition->faults) { 
    os << " faults[";
    DbFaultMap::iterator fault;
    for (fault = ignoreCondition->faults->begin();
        fault != std::prev(ignoreCondition->faults->end()); ++fault) {
        os << (*fault).second->id << ", ";
    }
    os << (*fault).second->id <<  "]";
  }

  return os;
}
