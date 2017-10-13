#ifndef CENTRAL_NODE_HISTORY_PROTOCOL_H
#define CENTRAL_NODE_HISTORY_PROTOROL_H

#include <stdint.h>
#include <iostream>

enum HistoryMessageType {
  FaultStateType = 1, // Fault change state (Faulted/Not Faulted)
  BypassStateType,    // Bypass expired/crated/cancelled
  BypassValueType,    // Bypass value changed
  MitigationType,     // Change in allowed beam class for mitigation device
  DeviceInputType,    // Change in digital input
  AnalogDeviceType,   // Change in analog device threshold status
};

typedef struct {
  HistoryMessageType type;
  uint32_t id;        // MPS database ID for the Fault/Input/Mitigation
  uint32_t oldValue;
  uint32_t newValue;
  uint32_t aux;

  void print() {
    std::cout << "[BYPAS] id=" << id
	      << ", old=" << oldValue
	      << ", new=" << newValue
	      << ", aux=" << aux << std::endl;
  }
} Message;


#endif
