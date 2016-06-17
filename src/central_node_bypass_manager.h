#ifndef CENTRAL_NODE_BYPASS_MANAGER_H
#define CENTRAL_NODE_BYPASS_MANAGER_H

#include <central_node_database.h>

typedef std::priority_queue<BypassQueueEntry, std::vector<BypassQueueEntry>, CompareBypassTime> BypassPriorityQueue;

class BypassManager {
 private:
  FaultInputBypassMapPtr bypassMap;
  BypassPriorityQueue bypassQueue;

 public:
  void createBypassMap(MpsDbPtr db);
  void assignBypass(MpsDbPtr db);
};

typedef shared_ptr<BypassManager> BypassManagerPtr;

#endif
