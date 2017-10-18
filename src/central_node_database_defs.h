#ifndef CENTRAL_NODE_DATABASE_DEFS_H
#define CENTRAL_NODE_DATABASE_DEFS_H

#include <bitset>
#include <stdint.h>

const uint32_t SLOW_EVALUATION = 0;
const uint32_t FAST_EVALUATION = 1;

// Definition of number of applications and memory space for
// the hardware (fast) configuration
const uint32_t NUM_APPLICATIONS = 1024;
const uint32_t APPLICATION_CONFIG_BUFFER_SIZE = 2048; // in bits - not all bits are used
const uint32_t APPLICATION_CONFIG_BUFFER_SIZE_BYTES = APPLICATION_CONFIG_BUFFER_SIZE / 8;
const uint32_t APPLICATION_CONFIG_BUFFER_USED_SIZE = 1344; // 1344 bits for digital, 1152 bits for analog
const uint32_t APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES = APPLICATION_CONFIG_BUFFER_USED_SIZE / 8; // 1344 bits for digital, 1152 bits for analog

//
// Application update buffer offsets
// Header
// [ -64 -   -1] Unknow 64-bit junk (!)
// [   0 -   64] timestamp
// [  64 -  127] 0x0000_0000_0000_0000_0000
// [ 128 -  511] App 0 status
// [ 512 -  896] App 1 status
// [ 897 - 1281] App 2 status
// [    ...    ] ... goes until App 1023
//
// App N status format:
// [   0 -  127] Was high status from bits 64 to 191
// [ 128 -  191] Was low status from bits 128 to 191
// [ 192 -  255] Was high status from bits 0 to 63
// [ 256 -  383] Was low status from bits 0 to 127
// 

// Header sizes
const uint32_t APPLICATION_UPDATE_BUFFER_HEADER_ZEROES_SIZE = 64;
const uint32_t APPLICATION_UPDATE_BUFFER_HEADER_TIMESTAMP_SIZE = 64;
const uint32_t APPLICATION_UPDATE_BUFFER_HEADER_SIZE =
  APPLICATION_UPDATE_BUFFER_HEADER_ZEROES_SIZE +
  APPLICATION_UPDATE_BUFFER_HEADER_TIMESTAMP_SIZE + 64;

const uint32_t APPLICATION_UPDATE_BUFFER_HEADER_ZEROES_SIZE_BYTES = APPLICATION_UPDATE_BUFFER_HEADER_ZEROES_SIZE / 8;
const uint32_t APPLICATION_UPDATE_BUFFER_HEADER_TIMESTAMP_SIZE_BYTES = APPLICATION_UPDATE_BUFFER_HEADER_TIMESTAMP_SIZE / 8;
const uint32_t APPLICATION_UPDATE_BUFFER_HEADER_SIZE_BYTES =
  APPLICATION_UPDATE_BUFFER_HEADER_ZEROES_SIZE_BYTES +
  APPLICATION_UPDATE_BUFFER_HEADER_TIMESTAMP_SIZE_BYTES + 8;

// Break 'was low'/'was high' bits into two section (small for 64 bits, large for 128 bits)
const uint32_t APPLICATION_UPDATE_BUFFER_64BITS = 64;
const uint32_t APPLICATION_UPDATE_BUFFER_128BITS = 128;
const uint32_t APPLICATION_UPDATE_BUFFER_64BITS_BYTES = 64 / 8;
const uint32_t APPLICATION_UPDATE_BUFFER_128BITS_BYTES = 128 / 8;
const uint32_t APPLICATION_UPDATE_BUFFER_INPUTS_SIZE =
  (APPLICATION_UPDATE_BUFFER_64BITS + APPLICATION_UPDATE_BUFFER_128BITS) * 2;
const uint32_t APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES = APPLICATION_UPDATE_BUFFER_INPUTS_SIZE / 8;

const uint32_t POWER_CLASS_BIT_SIZE = 4;
const uint32_t DESTINATION_MASK_BIT_SIZE = 16;
const uint32_t NUM_DESTINATIONS = 16;

// Constants for application cards
const uint32_t APP_CARD_MAX_ANALOG_CHANNELS = 6;
const uint32_t APP_CARD_MAX_DIGITAL_CHANNELS = 64;

// Offsets for firmware digital channel configuration
const uint32_t DIGITAL_CHANNEL_CONFIG_SIZE = 21; // in bits
const uint32_t DIGITAL_CHANNEL_POWER_CLASS_OFFSET = 0;
const uint32_t DIGITAL_CHANNEL_DESTINATION_MASK_OFFSET = 4; // in bits
const uint32_t DIGITAL_CHANNEL_EXPECTED_STATE_OFFSET = 20; // in bits

// Constants for analog channel configuration
const uint32_t ANALOG_CHANNEL_INTEGRATORS_SIZE = 8; // in bits
const uint32_t ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL = 4; // in bits
const uint32_t ANALOG_CHANNEL_DESTINATION_MASK_BASE = // in bits
  POWER_CLASS_BIT_SIZE *
  APP_CARD_MAX_ANALOG_CHANNELS *
  ANALOG_CHANNEL_INTEGRATORS_SIZE *
  ANALOG_CHANNEL_INTEGRATORS_PER_CHANNEL;
//const uint32_t ANALOG_CHANNEL_DESTINATION_MASK_SIZE = DESTINATION_MASK_BIT_SIZE *

// Constants for digital/analog input updates
// Bits   0 -  63: 0x0000_0000_0000_0000
// Bits  64 - 127: 64-bit timestamp
// Bits 128 - 223: 'was low' inputs (or is it 'was high'?)
// Bits 224 - 319: 'was high' inputs (or is it 'was low'?)
const uint32_t UPDATE_STATUS_BITS = 2; // 2 bits for each status, one for 'was Low' and one for 'was High'
const uint32_t DEVICE_INPUT_UPDATE_SIZE = 2; // in bits (one 'was Low'/'was High' per input)
const uint32_t ANALOG_DEVICE_NUM_THRESHOLDS = 32; // max number of thresholds per analog channel
const uint32_t ANALOG_DEVICE_UPDATE_SIZE = ANALOG_DEVICE_NUM_THRESHOLDS * 2; // in bits ('was Low'/'was High')
const uint32_t UPDATE_WAS_LOW_OFFSET = 0; // First 192 bits have the 'was Low' status
const uint32_t UPDATE_WAS_HIGH_OFFSET = 192; // Last 192 bits have the 'was High' status
const uint32_t DIGITAL_UPDATE_WAS_LOW_OFFSET = 64; // TODO: review offset // First 64 bits have the 'was Low' status
const uint32_t DIGITAL_UPDATE_WAS_HIGH_OFFSET = 0; // TODO: review offset // Last 64 bits have the 'was High' status

//
// Each application card has an array of bits for the fast firmware
// evaluation. The formats are different for digital and analog inputs
// 
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
//
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
typedef std::bitset<APPLICATION_CONFIG_BUFFER_SIZE> ApplicationConfigBufferBitSet;

typedef std::bitset<APPLICATION_UPDATE_BUFFER_INPUTS_SIZE * 6 +
                    APPLICATION_UPDATE_BUFFER_HEADER_SIZE> ApplicationUpdateBufferFullBitSet;

// Bit set with the 384 status bits from an application card
typedef std::bitset<APPLICATION_UPDATE_BUFFER_INPUTS_SIZE> ApplicationUpdateBufferBitSet;
typedef std::bitset<APPLICATION_UPDATE_BUFFER_INPUTS_SIZE/2> ApplicationUpdateBufferBitSetHalf;

typedef std::bitset<APPLICATION_UPDATE_BUFFER_128BITS> ApplicationUpdateBufferBitSetLarge;
typedef std::bitset<APPLICATION_UPDATE_BUFFER_64BITS> ApplicationUpdateBufferBitSetSmall;

#endif
