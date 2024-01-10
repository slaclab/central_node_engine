#ifndef CENTRAL_NODE_BYPASS_H
#define CENTRAL_NODE_BYPASS_H

#include <boost/shared_ptr.hpp>
#include <time.h>
#include <queue>
#include <stdint.h>

enum BypassType {
  BYPASS_DIGITAL,
  BYPASS_ANALOG,
  BYPASS_APPLICATION,
  BYPASS_FAULT
};

enum BypassStatus {
  BYPASS_VALID,
  BYPASS_EXPIRED
};

enum AnalogIntegratorIndex {
  BPM_X = 0,
  BPM_Y = 1,
  BPM_TMIT = 2,
  INT0 = 0,
  INT2 = 1,
  INT3 = 2,
  INT4 = 3
};

static const int BYPASS_DIGITAL_INDEX = 100;
static const int BYPASS_APPLICATION_INDEX = 200;


/**
 * Each FaultInput, AnalogChannel, applicationCard, fault have a pointer to a instance of InputBypass.
 *
 * For inputs that are evaluated in firmware the bypass must be changed in
 * the application configuration memory.
 *
 * The bypass value works only for slow evaluated channels. For fast evaluation
 * the input is ignored when bypassed, the bypass value is not used.
 *
 * It is enabled by writting zero to the 16-bit destination mask, effectively
 * disregarding the input state.
 */
class InputBypass {
 public:
  uint32_t id;

  // Index of the channel for this bypass
  uint32_t channelId;

  // Bypasses work for any applicationCard by setting its timeout enable to off.
  uint32_t appId; // Only set for applicationCard bypasses

  // Bypasses work for any fault by setting its channels values to reach a desired fault state.
  uint32_t faultId; // Only set for fault bypasses

  // This value is used to calculate the Fault value instead of the actual input value
  // Important: this bypass value is only used for slow evaluation.
  uint32_t value;

  // Defines the input type (analog or digital or applicationCard)
  // If digital then the faultInputId refers to a DbFaultInput entry
  // Otherwise the faultInputId refers to the DbAnalogchannel entry
  BypassType type;

  // Time in sec since 1970 when the bypass expires
  time_t until;

  // Indicates if bypass is still valid or not. The 'until' field is
  // checked once a second, while the 'status' field is used by the
  // engine to check whether the bypass value or the actual input value
  // should be used.
  BypassStatus status;

  // This index is used only for bypasses of Analogchannels, more specifically
  // it defines which threshold bit is bypassed. Each Analogchannel has up to
  // 32 InputBypass - each threshold can be bypassed individually
  // Analog integrator index (from 0 to 4)
  uint16_t index;
  uint32_t *bypassMask;

  // Indicates if this bypass requires firmware configuration update - this
  // is needed for inputs used by the fast rules
  bool configUpdate;

 InputBypass() : id(0), channelId(0), value(0),
    type(BYPASS_DIGITAL), until(0), status(BYPASS_EXPIRED),
    configUpdate(false) {
  }
};

typedef boost::shared_ptr<InputBypass> InputBypassPtr;
typedef std::map<int, InputBypassPtr> InputBypassMap;
typedef boost::shared_ptr<InputBypassMap> InputBypassMapPtr;

#endif
