#ifndef CENTRAL_NODE_BYPASS_H
#define CENTRAL_NODE_BYPASS_H

#include <boost/shared_ptr.hpp>
#include <time.h>
#include <queue>

using boost::shared_ptr;

enum BypassType {
  BYPASS_DIGITAL,
  BYPASS_ANALOG
};

class FaultInputBypass {
 public:
  int id;
  
  // Index of the fault input for this bypass
  int faultInputId;

  // This value is used to calculate the Fault value instead of the actual input value
  int value; 

  // Defines the input type (analog or digital)
  // If digital then the faultInputId refers to a DbFaultInput entry
  // Otherwise the faultInputId refers to the DbAnalogDevice entry
  BypassType type;

  // Time in sec since 1970 when the bypass expires
  time_t validUntil;

  // Indicates if bypass is still valid or not. The 'validUntil' field is
  // checked once a second, while the 'valid' field is used by the 
  // engine to check whether the bypass value or the actual input value
  // should be used.
  bool valid;

 FaultInputBypass() : id(-1), faultInputId(-1), value(0),
    type(BYPASS_DIGITAL), validUntil(0), valid(false) {
  }
};

typedef shared_ptr<FaultInputBypass> FaultInputBypassPtr;
typedef std::map<int, FaultInputBypassPtr> FaultInputBypassMap;
typedef shared_ptr<FaultInputBypassMap> FaultInputBypassMapPtr;

typedef std::pair<time_t, FaultInputBypassPtr> BypassQueueEntry;

class CompareBypassTime {
 public:
  bool operator()(BypassQueueEntry f1, BypassQueueEntry f2) {
    return f1.first > f2.first;
  }
};
/*
typedef std::priority_queue<BypassQueueEntry, std::vector<BypassQueueEntry>, CompareBypassTime> BypassPriorityQueue;

class BypassManager {
 private:
  FaultInputBypassMapPtr bypassMap;
  BypassPriorityQueue bypassQueue;

 public:
  void createBypassMap();
  void assignBypass(MpsDbPtr db);
};

typedef shared_ptr<BypassManager> BypassManagerPtr;
*/
#endif
