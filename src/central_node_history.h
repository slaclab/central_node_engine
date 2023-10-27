#ifndef CENTRAL_NODE_HISTORY_H
#define CENTRAL_NODE_HISTORY_H

#include <central_node_history_protocol.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <boost/shared_ptr.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef std::list<Message> HistoryQueue;

const uint32_t HIST_QUEUE_MAX_SIZE = 100;

class History {
 private:
  History();
  History(History const &);
  void operator=(History const &);
  std::thread *_senderThread;
  std::mutex _mutex;
  std::condition_variable _condVar;
  HistoryQueue _histQueue;
  Message message;
  int sock;
  int serverlen;
  struct sockaddr_in serveraddr;
  uint32_t _counter;
  bool _done;

 public:
  bool enabled;

  void startSenderThread(std::string serverName = "lcls-dev3", int port = 3356);
  void stopSenderThread();

  int log(HistoryMessageType type, uint32_t id, uint32_t oldValue, uint32_t newValue, uint32_t aux);
  int logFault(uint32_t id, uint32_t oldValue, uint32_t newValue, uint32_t allowedClass);
  int logDigitalChannel(uint32_t id, uint32_t oldValue, uint32_t newValue);
  int logAnalogChannel(uint32_t id, uint32_t oldValue, uint32_t newValue);
  int logBypassState(uint32_t id, uint32_t oldValue, uint32_t newValue);
  int add(Message &message);
  int send(Message &message);
  int sendFront();
  void senderThread();

  static History &getInstance() {
    static History instance;
    return instance;
  }

  friend std::ostream & operator<<(std::ostream &os, History * const history) {
    os << "=== History ===" << std::endl;
    os << "  messages sent: " << history->_counter << std::endl;

    return os;
  }

};

//typedef shared_ptr<History> HistoryPtr;

#endif
