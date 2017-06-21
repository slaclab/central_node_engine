#ifndef CENTRAL_NODE_BYPASS_H
#define CENTRAL_NODE_BYPASS_H

#include <boost/shared_ptr.hpp>
#include <time.h>
#include <queue>
#include <stdint.h>

using boost::shared_ptr;

enum BypassType {
  BYPASS_DIGITAL,
  BYPASS_ANALOG
};

enum BypassStatus {
  BYPASS_VALID,
  BYPASS_EXPIRED
};

/**
 * Each DeviceInput and AnalogDevice have a pointer to a instance of InputBypass.
 */
class InputBypass {
 public:
  uint32_t id;
  
  // Index of the device for this bypass
  uint32_t deviceId;

  // This value is used to calculate the Fault value instead of the actual input value
  uint32_t value; 

  // Defines the input type (analog or digital)
  // If digital then the faultInputId refers to a DbDeviceInput entry
  // Otherwise the faultInputId refers to the DbAnalogDevice entry
  BypassType type;

  // Time in sec since 1970 when the bypass expires
  time_t until;

  // Indicates if bypass is still valid or not. The 'validUntil' field is
  // checked once a second, while the 'valid' field is used by the 
  // engine to check whether the bypass value or the actual input value
  // should be used.
  BypassStatus status;

  // This index is used only for bypasses of AnalogDevices, more specifically
  // it defines which threshold bit is bypassed. Each AnalogDevice has up to
  // 32 InputBypass - each threshold can be bypassed individually
  uint16_t index;

 InputBypass() : id(0), deviceId(0), value(0),
    type(BYPASS_DIGITAL), until(0), status(BYPASS_EXPIRED) {
  }
};

typedef shared_ptr<InputBypass> InputBypassPtr;
typedef std::map<int, InputBypassPtr> InputBypassMap;
typedef shared_ptr<InputBypassMap> InputBypassMapPtr;

#endif
