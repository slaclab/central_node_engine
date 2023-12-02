/**
 * central_node_inputs.cc
 *
 * Functions responsible for updating buffers through reading FW 
 * for the following:
 * faultInputs, analog values (set of threshold bits), 
 * digital and analog configurations (beam class and destinations)
 */

#include <central_node_database_defs.h>
#include <central_node_database_tables.h>
#include <central_node_database.h>
#include <central_node_firmware.h>
#include <central_node_history.h>

#include <iostream>
#include <sstream>

#include <log_wrapper.h>

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
using namespace easyloggingpp;
static Logger *databaseLogger ;
#endif

TimeAverage DigitalChannelUpdateTime(5, "Digital Channel update time");
TimeAverage AnalogChannelUpdateTime(5, "Analog Channel update time");
TimeAverage AppCardDigitalUpdateTime(5, "Application Card (digital) update time");
TimeAverage AppCardAnalogUpdateTime(5, "Application Card (analog) update time");

ApplicationUpdateBufferBitSetHalf* DbApplicationCardInput::getWasLowBuffer()
{
    if (!fwUpdateBuffer)
        return NULL;

    return reinterpret_cast<ApplicationUpdateBufferBitSetHalf *>(&fwUpdateBuffer->at(wasLowBufferOffset));
}

ApplicationUpdateBufferBitSetHalf* DbApplicationCardInput::getWasHighBuffer()
{
    if (!fwUpdateBuffer)
        return NULL;

    return reinterpret_cast<ApplicationUpdateBufferBitSetHalf *>(&fwUpdateBuffer->at(wasHighBufferOffset));
}

uint32_t DbApplicationCardInput::getWasLow(int channel) {
  return (*getWasLowBuffer())[channel];
}

uint32_t DbApplicationCardInput::getWasHigh(int channel) {
  return (*getWasHighBuffer())[channel];
}

void DbApplicationCardInput::setUpdateBuffers(std::vector<uint8_t>* bufPtr, const size_t wasLowBufferOff, const size_t wasHighBufferOff)
{
    fwUpdateBuffer      = bufPtr;
    wasLowBufferOffset  = wasLowBufferOff;
    wasHighBufferOffset = wasHighBufferOff;
}

// void DbDeviceInput::setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer) {
//   applicationUpdateBuffer = buffer;
// }

// This should update the value from the data read from the central node firmware
void DbDigitalChannel::update(uint32_t v) {

  previousValue = value;
  value = v;

  // Latch new value if this is a fault
  if (v == faultValue) {
    latchedValue = faultValue;
  }
}

// Update its value from the applicationUpdateBuffer
void DbDigitalChannel::update() {

  uint32_t wasLow;
  uint32_t wasHigh;
  uint32_t newValue = 0;

  DigitalChannelUpdateTime.start();

  if (getWasLowBuffer()) {
    previousValue = value;

    wasLow = getWasLow(number);
    wasHigh = getWasHigh(number);

    wasLowBit = wasLow;
    wasHighBit = wasHigh;

    // If both are zero the Central Node has not received messages from the device, assume fault
    if (wasLow + wasHigh == 0) {
      invalidValueCount++;
      newValue = faultValue;
    }
    else if (wasLow + wasHigh == 2) {
      newValue = faultValue; // If signal was both low and high during the 2.7ms assume fault state.
    }
    else if (wasLow > 0) {
      newValue = 0;
    }
    else {
      newValue = 1;
    }

    value = newValue;

    // Latch new value if this is a fault
    if (newValue == faultValue) {
      latchedValue = faultValue;
    }
    if (auto_reset == AUTO_RESET) {
      latchedValue = value;
    }

    if (previousValue != value) {
      History::getInstance().logDigitalChannel(id, previousValue, value);
    }
  }
  else {
    throw(DbException("ERROR: DbDigitalChannel::update() - no applicationUpdateBuffer set"));
  }

  DigitalChannelUpdateTime.end();
}

DbAnalogChannel::DbAnalogChannel() : DbEntry(), number(-1),
				   value(0), previousValue(0),
				   invalidValueCount(0), ignored(false),
				   bypassMask(0xFFFFFFFF) {
  for (uint32_t i = 0; i < ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL; ++i) {
    fastDestinationMask[i] = 0;
    ignoredIntegrator[i] = false;
  }

  // Initialize power class to maximum
  for (uint32_t i = 0; i < ANALOG_CHANNEL_MAX_INTEGRATORS_PER_CHANNEL * ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
    fastPowerClass[i] = 0;
    fastPowerClassInit[i] = 1;
  }
}

// void DbAnalogDevice::setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer) {
//   applicationUpdateBuffer = buffer;
// }

void DbAnalogChannel::update(uint32_t v) {
  //    std::cout << "Updating analog device [" << id << "] value=" << v << std::endl;
  previousValue = value;
  value = v;

  // Latch new value if this there is a threshold at fault
  if ((value | latchedValue) != latchedValue) {
    latchedValue |= value;
  }
}

/**
 * Update the analog value (set of threshold bits) based on 'was High'/'was Low'
 * status read from firmware. An analog device has up to 4 integrators, with 8
 * comparators each, resulting in 32-bit thresholds masks. In the update buffer
 * each bit is represented by 2 bits (was High/was Low).
 *
 * The integrators for a given channel are not mapped contigously in update
 * buffer (see central_node_database_defs.h for more comments on the analog
 * update buffer format.)
 *
 * 'was Low' : threshold comparison is within limits, no faults
 * 'was High': threshold comparison outside limits, generate fault
 */
void DbAnalogChannel::update() {

  uint32_t wasLow;
  uint32_t wasHigh;
  uint32_t newValue = 0;

  AnalogChannelUpdateTime.start();

  if (getWasLowBuffer()) {
    previousValue = value;
    value = 0;

    uint32_t integratorOffset = 0;
    for (uint32_t i = 0; i < appType->numIntegrators; ++i) {
      integratorOffset = numChannelsCard * ANALOG_DEVICE_NUM_THRESHOLDS * i + number * ANALOG_DEVICE_NUM_THRESHOLDS;
      for (uint32_t j = 0; j < ANALOG_DEVICE_NUM_THRESHOLDS; ++j) {
	      wasLow = getWasLow(integratorOffset + j);
	      wasHigh = getWasHigh(integratorOffset + j);
        // If both are zero the Central Node has not received messages from the device, assume fault
	      // Both zeroes also mean no messages from application card in the last 360Hz period
	      if (wasLow + wasHigh == 0) {
	        invalidValueCount++;
	        newValue |= (1 << (j + i * ANALOG_DEVICE_NUM_THRESHOLDS)); // Threshold exceeded
	        latchedValue |= (1 << (j + i * ANALOG_DEVICE_NUM_THRESHOLDS));
	      }
	      else if (wasLow + wasHigh == 2) {
	        newValue |= (1 << (j + i * ANALOG_DEVICE_NUM_THRESHOLDS)); // If signal was both low and high during the 2.7ms set threshold crossed
	        latchedValue |= (1 << (j + i * ANALOG_DEVICE_NUM_THRESHOLDS));
	      }
	      else if (wasHigh > 0) {
	        newValue |= (1 << (j + i * ANALOG_DEVICE_NUM_THRESHOLDS)); // Threshold exceeded
	        latchedValue |= (1 << (j + i * ANALOG_DEVICE_NUM_THRESHOLDS));
	      }
	      else {
	        newValue &= ~(1 << (j + i * ANALOG_DEVICE_NUM_THRESHOLDS)); // No threshold crossed
	      }
      }
    }

    /*
    // TODO: Change the bit mapping - the integrators are not contiguous in memory! This change is dependent on FW people
    for (uint32_t i = 0; i < ANALOG_DEVICE_NUM_THRESHOLDS * deviceType->numIntegrators; ++i) {
      wasLow = getWasLow(channel->number*ANALOG_DEVICE_NUM_THRESHOLDS*deviceType->numIntegrators+i);
      wasHigh = getWasHigh(channel->number*ANALOG_DEVICE_NUM_THRESHOLDS*deviceType->numIntegrators+i);

      // If both are zero the Central Node has not received messages from the device, assume fault
      // Both zeroes also mean no messages from application card in the last 360Hz period
      if (wasLow + wasHigh == 0) {
	invalidValueCount++;
	newValue |= (1 << i); // Threshold exceeded
	latchedValue |= (1 << i);
      }
      else if (wasLow + wasHigh == 2) {
	newValue |= (1 << i); // If signal was both low and high during the 2.7ms set threshold crossed
	latchedValue |= (1 << i);
      }
      else if (wasHigh > 0) {
	newValue |= (1 << i); // Threshold exceeded
	latchedValue |= (1 << i);
      }
      else {
	newValue &= ~(1 << i); // No threshold crossed
      }
    }
    */
    value = newValue;
    //    std::cout << name << ": " <<  value << std::endl;

    if (previousValue != value) {
      History::getInstance().logAnalogChannel(id, previousValue, value);
    }
  }
  else {
    throw(DbException("ERROR: DbAnalogDevice::update() - no applicationUpdateBuffer set"));
  }

  AnalogChannelUpdateTime.end();
}

/**
 * Assign the applicationUpdateBuffer to all digital/analog channels, which is
 * used to retrieve the latest updates from the firmware
 */
void DbApplicationCard::configureUpdateBuffers() {

 std::stringstream errorStream;
  //  std::cout << this << std::endl;
  if (digitalChannels) {
    for (DbDigitalChannelMap::iterator digitalChannel = digitalChannels->begin();
      digitalChannel != digitalChannels->end(); ++digitalChannel) {
      if ((*digitalChannel).second->evaluation != NO_EVALUATION &&
        !(*digitalChannel).second->faultInputs) {
        errorStream << "ERROR: Found digital channel (" << (*digitalChannel).second->name << ") "
        << "without inputs (eval=" << (*digitalChannel).second->evaluation << ")";
        throw(DbException(errorStream.str()));
      }
      uint32_t cardId = (*digitalChannel).second->cardId;
      if (cardId == number) {
        (*digitalChannel).second->setUpdateBuffers(fwUpdateBuffer, wasLowBufferOffset, wasHighBufferOffset);
        (*digitalChannel).second->configured = true;
      }
      else {
        (*digitalChannel).second->configured = false;
        std::cout << "INFO: Digital Channel " << (*digitalChannel).second->name << " not in this application card.  Configure later..." << std::endl;
      }
    }
  }
  else if (analogChannels) {
    for (DbAnalogChannelMap::iterator analogChannel = analogChannels->begin();
	    analogChannel != analogChannels->end(); ++analogChannel) {
      //      std::cout << "A" << (*analogChannel).second->id << " ";
      //(*analogChannel).second->setUpdateBuffer(applicationUpdateBuffer);
      (*analogChannel).second->setUpdateBuffers(fwUpdateBuffer, wasLowBufferOffset, wasHighBufferOffset);
    }
  }
  else {
    //    throw(DbException("Can't configure update buffers - no devices configured"));
    std::cerr << "WARN: No devices configured for application card " << applicationType->name
	      << " (Id: " << this->id << ")" << std::endl;
  }
}

std::vector<uint8_t>* DbApplicationCard::getFwUpdateBuffer() {
  return fwUpdateBuffer;
}

size_t DbApplicationCard::getWasLowBufferOffset() {
  return wasLowBufferOffset;
}

size_t DbApplicationCard::getWasHighBufferOffset() {
  return wasHighBufferOffset;
}

/**
 * Once the applicationUpdateBuffer has been updated with firmware status then
 * update each digital/analog device with the new values.
 */
bool DbApplicationCard::updateInputs() {
  // Check if timeout status bit from firmware is on, if so set online to false
  if (Firmware::getInstance().getAppTimeoutStatus(number)) {
    online = false;
  }
  else {
    online = true;
  }
  // 2 use cases:
  // 1) Bypass is for human operator bypasses with the information to safely bypass an app card
  // 2) ignore is set in the epics PV interface (through centralNodeDriver eventually to central node) 
    // And it is used for if we SWITCH to NC, then all these app cards in SC will fault since it won't
    // meet the 1MHz time, it'll essentially timeout. Which is why we have to set the timeout enable to false 
    // when NC mode is on. 
  if (bypass->status == BYPASS_VALID) { 
    if (!bypassed) {
      Firmware::getInstance().setAppTimeoutEnable(id, false, false); // Don't write to fw, since the active != oldactive will cause a reload ahead
      bypassed = true;
    }
  }
  else {
    if (bypassed) {
      Firmware::getInstance().setAppTimeoutEnable(id, true, false);
      bypassed = false;
    }
  }

  // Ignore condition check - seperate from bypass because it is not a result of human operator bypass
  if (ignoreStatus == true) { 
    if (!ignored) {
        Firmware::getInstance().setAppTimeoutEnable(id, false, false); // Don't write to fw, since the active != oldactive will cause a reload ahead
        ignored = true;
    }
  }
  else {
    if (ignored) {
      Firmware::getInstance().setAppTimeoutEnable(id, true, false);
      ignored = false;
    }

  }

  bool reload = false;
  bool oldActive = active;
  if (Firmware::getInstance().getAppTimeoutEnable(number)) {
    active = true;
  }
  else {
    active = false;
  }

  if(active != oldActive) {
    reload = true;
  }
  if (digitalChannels) {
    AppCardDigitalUpdateTime.start();
    for (DbDigitalChannelMap::iterator digitalChannel = digitalChannels->begin();
	       digitalChannel != digitalChannels->end(); ++digitalChannel) {
      (*digitalChannel).second->faultedOffline = !online; //true when it is faulted offline
      (*digitalChannel).second->modeActive = active; //True when SC mode, false when NC mode
      (*digitalChannel).second->update();
    }
    AppCardDigitalUpdateTime.end();
  }
  else if (analogChannels) {
    AppCardAnalogUpdateTime.start();

    for (DbAnalogChannelMap::iterator analogChannel = analogChannels->begin();
	       analogChannel != analogChannels->end(); ++analogChannel) {
      (*analogChannel).second->update();
      (*analogChannel).second->faultedOffline = !online; //true when it is faulted offline
      (*analogChannel).second->modeActive = active; //True when SC mode, false when NC mode
    }
    AppCardAnalogUpdateTime.end();
  }
  else {
    //    throw(DbException("Can't configure update devices because there are no devices"));
    //    std::cerr << "WARN: No devices configured for application card " << this->name
    //	      << " (Id: " << this->id << ")" << std::endl;
  }
  return reload;
}

/**
 *
 */
void DbApplicationCard::writeConfiguration(bool enableTimeout) {

  if (digitalChannels) {
    writeDigitalConfiguration();
    hasInputs = true;
  }
  else if (analogChannels) {
    writeAnalogConfiguration();
    hasInputs = true;
  }
  else {
    //    throw(DbException("Can't configure application card - no devices configured"));
    //    std::cerr << "WARN: No devices configured for application card " << this->name
    //	      << " (Id: " << this->id << ")" << std::endl;
    hasInputs = false;
    return;
  }
  // Enable application timeout
  if (enableTimeout) {
    Firmware::getInstance().setAppTimeoutEnable(number, true, false);
  }
}

// Digital input configuration (total size = 1344 bits):
//
//   +------------> Expected digital input state
//   |   +--------> 16-bit destination mask
//   |   |    +---> 4-bit encoded power class
//   |   |    |
// +---+----+---+---        +---+----+---+---+----+---+
// | 1 | 16 | 4 |    ...    | 1 | 16 | 4 | 1 | 16 | 4 |
// +---+----+---+---     ---+---+----+---+---+----+---+
// |    M63     |    ...    |     M1     |     M0     |
//
// where Mxx is the mask for bit xx
void DbApplicationCard::writeDigitalConfiguration() {

  // First set all bits to zero
  applicationConfigBuffer->reset();

  std::stringstream errorStream;
  for (DbDigitalChannelMap::iterator digitalChannel = digitalChannels->begin();
      digitalChannel != digitalChannels->end(); ++digitalChannel) {
    // Only configure firmware for devices/faults that require fast evaluation -
    // A DigitalChannel with evaluation set to FAST_EVALUATION *must* have only
    // one FaultInput.
    if ((*digitalChannel).second->evaluation == FAST_EVALUATION) {
      if ((*digitalChannel).second->faultInputs->size() == 1) {
        DbFaultInputMap::iterator faultInput = (*digitalChannel).second->faultInputs->begin();

        // Write the ExpectedState (index 20)
        int channelNumber = (*digitalChannel).second->number;
        int channelOffset = channelNumber * DIGITAL_CHANNEL_CONFIG_SIZE;
        int offset = DIGITAL_CHANNEL_EXPECTED_STATE_OFFSET;
        applicationConfigBuffer->set(channelOffset + offset, (*digitalChannel).second->fastExpectedState);

        // Write the destination mask (index 4 through 19)
        // If bypass for device is valid leave destination mask set to zero - i.e. no mitigation
        if ((*faultInput).second->digitalChannel->bypass->status != BYPASS_VALID) {
          offset = DIGITAL_CHANNEL_DESTINATION_MASK_OFFSET;
          for (uint32_t i = 0; i < DESTINATION_MASK_BIT_SIZE; ++i) {
            bool bit = ((*digitalChannel).second->fastDestinationMask >> i) & 0x01;
            applicationConfigBuffer->set(channelOffset + offset + i, bit);
          }
        }

        // Write the beam power class (index 0 through 3)
        offset = DIGITAL_CHANNEL_POWER_CLASS_OFFSET;
        for (uint32_t i = 0; i < POWER_CLASS_BIT_SIZE; ++i) {
          applicationConfigBuffer->set(channelOffset + offset + i,
              ((*digitalChannel).second->fastPowerClass >> i) & 0x01);
        }
      }
      else {
        errorStream << "ERROR: DigitalChannel configured with FAST evaluation must have one input only."
        << " Found " << (*digitalChannel).second->faultInputs->size() << " inputs for "
        << "device " << (*digitalChannel).second->name;
        throw(DbException(errorStream.str()));
      }
    }
  }
}

// Analog input configuration (total size = 1152 bits):
//
//   +------------> 16-bit destination mask
//   |
//   |                        +---> 4-bit encoded power class
//   |                        |
// +----+----+---   ---+----+----+---+---   ---+---+---+
// | 16 | 16 |   ...   | 16 | 4  | 4 |   ...   | 4 | 4 |
// +----+----+---   ---+----+----+---+---   ---+---+---+
// |B23 |B22 |   ...   | B0 |M191|M190   ...   | M1| M0|
//
// Bxx are the 16-bit destination masks for each
// 4 integrators of each of the 6 channels:
// 6 * 4 = 24 (B0-B23)
//
// Power classes (M0-M191):
// 4 integrators per channel (BPM has only 3 - X, Y and TMIT)
// 8 comparators for each integrator:
// 8 * 4 * 6 = 192 (M0 through M191)
//
// The number of integrators vary according to the devices. The
// DbApplicationType->numIntegrators have the number of integrators
// defined for the device.
//
// ***
// EIC Version: there is only one integrator per channel, therefore
// only B0 though B6 are actually used
// ***
void DbApplicationCard::writeAnalogConfiguration() {

  // First set all bits to zero
  applicationConfigBuffer->reset();

  std::stringstream errorStream;

  // Loop through all Analog Channels within this Application
  for (DbAnalogChannelMap::iterator analogChannel = analogChannels->begin();
       analogChannel != analogChannels->end(); ++analogChannel) {
    // Only configure firmware for devices/faults that require fast evaluation
    if ((*analogChannel).second->evaluation == FAST_EVALUATION) {
      LOG_TRACE("DATABASE", "AnalogConfig: " << (*analogChannel).second->name);
      //      std::cout << "AnalogConfig: " << (*analogChannel).second->name << std::endl;

      int channelNumber = (*analogChannel).second->number;

      // Write power classes for each threshold bit - the integratorsPerChannel for all
      // devices must be the same
      uint32_t integratorsPerChannel = (*analogChannel).second->appType->numIntegrators;
      uint32_t powerClassOffset = 0;
      uint32_t channelsPerCard = (*analogChannel).second->numChannelsCard;

      for (uint32_t i = 0; i < integratorsPerChannel; ++i) { // for each integrator
	      powerClassOffset = channelNumber * ANALOG_DEVICE_NUM_THRESHOLDS * POWER_CLASS_BIT_SIZE +
	                         i * channelsPerCard * ANALOG_DEVICE_NUM_THRESHOLDS * POWER_CLASS_BIT_SIZE;
	      for (uint32_t j = 0; j < ANALOG_DEVICE_NUM_THRESHOLDS; ++j) { // for each threshold
	        for (uint32_t k = 0; k < POWER_CLASS_BIT_SIZE; ++k) { // for each power class bit
	          applicationConfigBuffer->set(powerClassOffset + k,
					      ((*analogChannel).second->fastPowerClass[j + i * ANALOG_DEVICE_NUM_THRESHOLDS] >> k) & 0x01);
            //std::cout << "offset=" << powerClassOffset+k << ", bit=" << (((*analogChannel).second->fastPowerClass[j] >> k) & 0x01) << std::endl;
	        }
	        powerClassOffset += POWER_CLASS_BIT_SIZE;
	      }
      }

      // Write the destination mask for each integrator
      uint32_t maskOffset = 0;
      for (uint32_t i = 0; i < integratorsPerChannel; ++i) { // for each integrator
        maskOffset = ANALOG_CHANNEL_DESTINATION_MASK_BASE +
          channelNumber * DESTINATION_MASK_BIT_SIZE +
          i * channelsPerCard * DESTINATION_MASK_BIT_SIZE;
        for (uint32_t j = 0; j < DESTINATION_MASK_BIT_SIZE; ++j) { // for each threshold
          bool bitValue = false;
          if (((*analogChannel).second->fastDestinationMask[i] >> j) & 0x01) {
            bitValue = true;
          }

          // If bypass for the integrator is valid, set destination mask to zero - i.e. no mitigation
          // No mitigation also if the analogChannel is currently ignored
          if ((*analogChannel).second->bypass[i]->status == BYPASS_VALID ||
              (*analogChannel).second->ignoredIntegrator[i] ||
              (*analogChannel).second->ignored) {
            bitValue = false;
          }
          applicationConfigBuffer->set(maskOffset + j, bitValue);
          //std::cout << "offset=" << maskOffset+j << ", bit=" << bitValue << "(ignored=" << (*analogChannel).second->ignored<<  ")" << std::endl;
        }
      }
    }
  }
  // Print bitset in string format
  //  std::cout << name << " [" << description << "]: ";
  //  std::cout << *applicationConfigBuffer << std::endl;
}

void DbApplicationCard::printAnalogConfiguration() {
  /*
  // First print the 24 destination masks
  std::cout << "Destination Masks:" << std::endl;
  for (int i = 0; i < 24; ++i) {
    std::cout << i << "\t";
  }
  std::cout << std::endl;
  int index=0;
  for (int i = 0; i < 24; ++i) {
    int16_t mask = 0;
    for (uint32_t j = 0; j < FW_NUM_BEAM_DESTINATIONS; ++j) {
      mask <<= 1;
      mask |= ((*applicationConfigBuffer)[index] & 1);
      index++;
    }
    std::cout << "0x" << std::hex << mask << std::dec << "\t";
  }
  std::cout << std::endl;
  */
}

bool DbApplicationCard::isAnalog() {
  return analogChannels != 0;
}

bool DbApplicationCard::isDigital() {
  return digitalChannels != 0;
}
