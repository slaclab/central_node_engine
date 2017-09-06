#ifndef CENTRAL_NODE_HISTORY_H
#define CENTRAL_NODE_HISTORY_H

#include <central_node_history_protocol.h>

#include <iostream>
#include <pthread.h>
#include <mqueue.h>
#include <boost/shared_ptr.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

class History {
 private:
  History();
  History(History const &);
  void operator=(History const &);
  pthread_t _senderThread;
  pthread_mutex_t mutex;
  mqd_t historyQueue;
  Message message;
  int sock;
  int serverlen;
  struct sockaddr_in serveraddr;

 public:
  void startSenderThread();

  int log(MessageType type, uint32_t id, uint32_t oldValue, uint32_t newValue, uint32_t aux);

  int logFault(uint32_t id, uint32_t oldValue, uint32_t newValue, uint32_t faultStateId);
  int logMitigation(uint32_t id, uint32_t oldValue, uint32_t newValue);
  int logDeviceInput(uint32_t id, uint32_t oldValue, uint32_t newValue);
  int logAnalogDevice(uint32_t id, uint32_t oldValue, uint32_t newValue);
  int logBypassState(uint32_t id, uint32_t oldValue, uint32_t newValue);
  int logBypassValue(uint32_t id, uint32_t oldValue, uint32_t newValue);
  int add(Message &message);
  int send(Message &message);
  static void *senderThread(void *arg);

  static History &getInstance() {
    static History instance;
    return instance;
  }

  friend std::ostream & operator<<(std::ostream &os, History * const history) {
    os << "=== History ===" << std::endl;

    return os;
  }

};

//typedef shared_ptr<History> HistoryPtr;

#endif
