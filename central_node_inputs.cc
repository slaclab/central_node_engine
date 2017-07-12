#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/emit.h>

#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_firmware.h>

#include <iostream>
#include <sstream>

#include <log_wrapper.h>

#ifdef LOG_ENABLED
using namespace easyloggingpp;
static Logger *databaseLogger ;
#endif

void DbDeviceInput::setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer) {
  applicationUpdateBuffer = buffer;
}

// This should update the value from the data read from the central node firmware
void DbDeviceInput::update(uint32_t v) {
  value = v;
  
  // Latch new value if this is a fault
  if (v == faultValue) {
    latchedValue = faultValue;
  }
}

// Update its value from the applicationUpdateBuffer
void DbDeviceInput::update() {
  uint32_t wasLow;
  uint32_t wasHigh;
  uint32_t newValue;
  
  if (applicationUpdateBuffer) {
    wasLow = (*applicationUpdateBuffer)[channel->number * DEVICE_INPUT_UPDATE_SIZE];
    wasHigh = (*applicationUpdateBuffer)[channel->number * DEVICE_INPUT_UPDATE_SIZE + 1];
    if (wasLow + wasHigh == 0) {
      invalidValueCount++;
      /*
      std::stringstream errorStream;
      errorStream << "ERROR: Read wasLow=0 and wasHigh=0 for digital channel " << channel->name;
      throw(DbException(errorStream.str())); 
      */
    }
    else if (wasLow + wasHigh == 2) {
      value = faultValue; // If signal was both low and high during the 2.7ms assume fault state.
    }
    else if (wasLow > 0) {
      newValue = 1;
    }
    else {
      newValue = 0;
    }

    value = newValue;
  
    // Latch new value if this is a fault
    if (newValue == faultValue) {
      latchedValue = faultValue;
    }
  }
  else {
    throw(DbException("ERROR: DbDeviceInput::update() - no applicationUpdateBuffer set"));
  }
}

DbAnalogDevice::DbAnalogDevice() : DbEntry(), deviceTypeId(-1), channelId(-1),
				   value(0), invalidValueCount(0), bypassMask(0xFFFFFFFF) {
  for (int i = 0; i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL; ++i) {
    fastDestinationMask[i] = 0;
  }
  
  for (int i = 0; i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL * ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
    fastPowerClass[i] = 0;
  }
}

void DbAnalogDevice::setUpdateBuffer(ApplicationUpdateBufferBitSet *buffer) {
  applicationUpdateBuffer = buffer;
}

void DbAnalogDevice::update(uint32_t v) {
  //    std::cout << "Updating analog device [" << id << "] value=" << v << std::endl;
  value = v;
  
  // Latch new value if this there is a threshold at fault
  if (value | latchedValue != latchedValue) {
    latchedValue |= value;
  }
}

/**
 * Update the analog value (set of threshold bits) based on 'was High'/'was Low'
 * status read from firmware. An analog device has up to 4 integrators, with 8 
 * comparators each, resulting in 32-bit thresholds masks. In the update buffer
 * each bit is represented by 2 bits (was High/was Low).
 */
void DbAnalogDevice::update() {
  uint32_t wasLow;
  uint32_t wasHigh;
  uint32_t newValue = 0;
  uint32_t newLatchedValue = latchedValue;
  
  if (applicationUpdateBuffer) {
    for (int i = 0; i < ANALOG_DEVICE_NUM_THRESHOLDS; ++i) {
      wasLow = (*applicationUpdateBuffer)[channel->number * ANALOG_DEVICE_UPDATE_SIZE + i * UPDATE_STATUS_BITS];
      wasHigh = (*applicationUpdateBuffer)[channel->number * ANALOG_DEVICE_UPDATE_SIZE + i * UPDATE_STATUS_BITS + 1];
      if (wasLow + wasHigh == 0) {
	invalidValueCount++;
	/*
	std::stringstream errorStream;
	errorStream << "ERROR: Read wasLow=0 and wasHigh=0 for analog channel " << channel->name;
	throw(DbException(errorStream.str())); 
	*/
      }
      else if (wasLow + wasHigh == 2) {
	newValue |= (1 << i); // If signal was both low and high during the 2.7ms set threshold crossed
	latchedValue |= (1 << i);
      }
      else if (wasLow > 0) {
	newValue |= (1 << i); // Threshold exceeded
	latchedValue |= (1 << i);
      }
      else {
	newValue &= ~(1 << i); // No threshold crossed
      }
    }
  }
  else {
    throw(DbException("ERROR: DbAnalogDevice::update() - no applicationUpdateBuffer set"));
  }
}

/**
 * Assign the applicationUpdateBuffer to all digital/analog channels, which is
 * used to retrieve the latest updates form the firmware
 */
void DbApplicationCard::configureUpdateBuffers() {
  //  std::cout << this << std::endl;
  if (digitalDevices) {
    for (DbDigitalDeviceMap::iterator digitalDevice = digitalDevices->begin();
	 digitalDevice != digitalDevices->end(); ++digitalDevice) {
      for (DbDeviceInputMap::iterator deviceInput = (*digitalDevice).second->inputDevices->begin();
	   deviceInput != (*digitalDevice).second->inputDevices->end(); ++deviceInput) {
	//	std::cout << "D" << (*deviceInput).second->id << " ";
	(*deviceInput).second->setUpdateBuffer(applicationUpdateBuffer);
      }
    }
  }
  else if (analogDevices) {
    for (DbAnalogDeviceMap::iterator analogDevice = analogDevices->begin();
	 analogDevice != analogDevices->end(); ++analogDevice) {
      //      std::cout << "A" << (*analogDevice).second->id << " ";
      (*analogDevice).second->setUpdateBuffer(applicationUpdateBuffer);
    }
  }
  else {
    throw(DbException("Can't configure update buffers - no devices configured"));
  }
}

/**
 * Once the applicationUpdateBuffer has been updated with firmware status then
 * update each digital/analog device with the new values.
 */
void DbApplicationCard::updateInputs() {
  if (digitalDevices) {
    for (DbDigitalDeviceMap::iterator digitalDevice = digitalDevices->begin();
	 digitalDevice != digitalDevices->end(); ++digitalDevice) {
      for (DbDeviceInputMap::iterator deviceInput = (*digitalDevice).second->inputDevices->begin();
	   deviceInput != (*digitalDevice).second->inputDevices->end(); ++deviceInput) {
	(*deviceInput).second->update();
      }
    }
  }
  else if (analogDevices) {
    for (DbAnalogDeviceMap::iterator analogDevice = analogDevices->begin();
	 analogDevice != analogDevices->end(); ++analogDevice) {
      (*analogDevice).second->update();
    }
  }
  else {
    throw(DbException("Can't configure update devices because there are no devices"));
  }
}

/**
 * 
 */
void DbApplicationCard::writeConfiguration() {
  if (digitalDevices) {
    writeDigitalConfiguration();
  }
  else if (analogDevices) {
    writeAnalogConfiguration();
  }
  else {
    throw(DbException("Can't configure application card - no devices configured"));
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
  for (DbDigitalDeviceMap::iterator digitalDevice = digitalDevices->begin();
       digitalDevice != digitalDevices->end(); ++digitalDevice) {
    // Only configure firmware for devices/faults that require fast evaluation
    if ((*digitalDevice).second->evaluation == FAST_EVALUATION) {
      if ((*digitalDevice).second->inputDevices->size() == 1) {
	DbDeviceInputMap::iterator deviceInput = (*digitalDevice).second->inputDevices->begin();

	// Write the ExpectedState (index 20)
	int channelNumber = (*deviceInput).second->channel->number;
	int channelOffset = channelNumber * DIGITAL_CHANNEL_CONFIG_SIZE;
	int offset = DIGITAL_CHANNEL_EXPECTED_STATE_OFFSET;
	applicationConfigBuffer->set(channelOffset + offset, (*digitalDevice).second->fastExpectedState);

	// Write the destination mask (index 4 through 19)
	offset = DIGITAL_CHANNEL_DESTINATION_MASK_OFFSET;
	for (int i = 0; i < DESTINATION_MASK_BIT_SIZE; ++i) {
	  applicationConfigBuffer->set(channelOffset + offset + i,
				 ((*digitalDevice).second->fastDestinationMask >> i) & 0x01);
	}

	// Write the beam power class (index 0 through 3)
	offset = DIGITAL_CHANNEL_POWER_CLASS_OFFSET;
	for (int i = 0; i < POWER_CLASS_BIT_SIZE; ++i) {
	  applicationConfigBuffer->set(channelOffset + offset + i,
				 ((*digitalDevice).second->fastPowerClass >> i) & 0x01);
	}
      }
      else {
	errorStream << "ERROR: DigitalDevice configured with FAST evaluation must have one input only."
		    << " Found " << (*digitalDevice).second->inputDevices->size() << " inputs for "
		    << "device " << (*digitalDevice).second->name;
	throw(DbException(errorStream.str())); 
      }
    }
  }
  // Print bitset in string format
  //  std::cout << *applicationConfigBuffer << std::endl;

  //  Firmware::getInstance().writeConfig(globalId);
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
void DbApplicationCard::writeAnalogConfiguration() {
  // First set all bits to zero
  applicationConfigBuffer->reset();
  
  std::stringstream errorStream;

  // Loop through all Analog Devices within this Application
  for (DbAnalogDeviceMap::iterator analogDevice = analogDevices->begin();
       analogDevice != analogDevices->end(); ++analogDevice) {
    // Only configure firmware for devices/faults that require fast evaluation
    if ((*analogDevice).second->evaluation == FAST_EVALUATION) {
      LOG_TRACE("DATABASE", "AnalogConfig: " << (*analogDevice).second->name);
      //      std::cout << (*analogDevice).second << std::endl;
      int channelNumber = (*analogDevice).second->channel->number;

      // Write power classes for each threshold bit
      uint32_t powerClassOffset = channelNumber *
	ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL * POWER_CLASS_BIT_SIZE;
      for (int i = 0; i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL *
	     ANALOG_CHANNEL_INTEGRATORS_SIZE; ++i) {
	for (int j = 0; j < POWER_CLASS_BIT_SIZE; ++j) {
	  applicationConfigBuffer->set(powerClassOffset + j,
				 ((*analogDevice).second->fastPowerClass[i] >> j) & 0x01);
	}
	powerClassOffset += POWER_CLASS_BIT_SIZE;
      }
      
      // Write the destination mask for each integrator
      int offset = ANALOG_CHANNEL_DESTINATION_MASK_BASE +
	channelNumber * ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL * DESTINATION_MASK_BIT_SIZE;
      for (int i = 0; i < ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL; ++i) {
	for (int j = 0; j < DESTINATION_MASK_BIT_SIZE; ++j) {
	  applicationConfigBuffer->set(offset + j,
				 ((*analogDevice).second->fastDestinationMask[i] >> j) & 0x01);
	}
	offset += DESTINATION_MASK_BIT_SIZE;
      }
    }
  }
  // Print bitset in string format
  //  std::cout << *applicationConfigBuffer << std::endl;
}

bool DbApplicationCard::isAnalog() {
  return analogDevices != 0;
}

bool DbApplicationCard::isDigital() {
  return digitalDevices != 0;
}
