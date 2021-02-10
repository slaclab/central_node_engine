#ifndef CENTRAL_NODE_BYPASS_MANAGER_H
#define CENTRAL_NODE_BYPASS_MANAGER_H

#include <central_node_database_tables.h>
#include <central_node_database.h>
#include <stdint.h>
#include <thread>

/**
 * The bypass expirations are monitored via a priority queue. The head of the
 * queue points to the bypass that will expire first. If a bypass time is
 *
 */
typedef std::pair<time_t, InputBypassPtr> BypassQueueEntry;

class CompareBypassTime {
 public:
  bool operator()(BypassQueueEntry f1, BypassQueueEntry f2) {
    return f1.first > f2.first;
  }
};

typedef std::priority_queue<BypassQueueEntry,
  std::vector<BypassQueueEntry>, CompareBypassTime> BypassPriorityQueue;

class BypassManager {
 private:
  InputBypassMapPtr bypassMap;
  BypassPriorityQueue bypassQueue;

  bool checkBypassQueueTop(time_t now);

  bool threadDone;
  std::thread *_bypassThread;
  std::mutex mutex;
  bool initialized;
  static bool refreshFirmwareConfiguration;

 public:
  BypassManager();
  ~BypassManager();
  void createBypassMap(MpsDbPtr db);
  void assignBypass(MpsDbPtr db);
  void checkBypassQueue(time_t testTime = 0);
  void setThresholdBypass(MpsDbPtr db, BypassType bypassType,
			  uint32_t deviceId, uint32_t value, time_t bypassUntil,
			  int thresholdIndex, bool test = false);
  void setBypass(MpsDbPtr db, BypassType bypassType, uint32_t deviceId,
		 uint32_t value, time_t bypassUntil, bool test = false);
  void printBypassQueue();
  bool isInitialized();

  void startBypassThread();
  void stopBypassThread();
  void bypassThread();

  friend class BypassTest;
};

typedef boost::shared_ptr<BypassManager> BypassManagerPtr;

#endif
