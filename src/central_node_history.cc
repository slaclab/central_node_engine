#include <central_node_history.h>
#include <central_node_exception.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include <netdb.h>

History::History() : _counter(0), _done(false), enabled(true) {
}

void History::startSenderThread(std::string serverName, int port) {
  struct hostent *server;

  sock = socket(AF_INET, SOCK_DGRAM, 0);

  if (sock < 0) {
    throw(CentralNodeException("ERROR: Failed to create socket for MPS history sender."));
  }

  server = gethostbyname(serverName.c_str());
  if (server == NULL) {
    throw(CentralNodeException("ERROR: Failed to create socket for MPS history sender."));
  }

  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
	(char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(port);
  serverlen = sizeof(serveraddr);

  if (enabled) {

    _senderThread = new std::thread(&History::senderThread, this);
    std::cout << "INFO: MPS History sender ready" << std::endl;

    if(pthread_setname_np(_senderThread->native_handle(), "SenderThread")) {
      perror("pthread_setname_np failed");
    }
  }

}

int History::log(HistoryMessageType type, uint32_t id, uint32_t oldValue, uint32_t newValue, uint32_t aux) {
  message.id = id;
  message.oldValue = oldValue;
  message.newValue = newValue;
  message.type = type;
  message.aux = aux; // aux is optional, set to 0 if not needed

  return add(message);
}

int History::logFault(uint32_t id, uint32_t oldValue, uint32_t newValue, uint32_t allowedClass) {
  return log(FaultStateType, id, oldValue, newValue, allowedClass);
}

int History::logDigitalChannel(uint32_t id, uint32_t oldValue, uint32_t newValue) {
  return log(DigitalChannelType, id, oldValue, newValue, 0);
}

int History::logAnalogChannel(uint32_t id, uint32_t oldValue, uint32_t newValue) {
  return log(AnalogChannelType, id, oldValue, newValue, 0);
}

int History::logBypassState(uint32_t id, uint32_t oldValue, uint32_t newValue, uint16_t index) {
  return log(BypassStateType, id, oldValue, newValue, index);
}

int History::add(Message &message) {
  if (!enabled) {
    return 1;
  }

  {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_histQueue.size() >= HIST_QUEUE_MAX_SIZE) {
      return 1;
    }
    _histQueue.push_back(message);
    _condVar.notify_all();
  }

  return 0;
}

void History::stopSenderThread() {
  std::cout << "INFO: Stopping history thread" << std::endl;
  _done = true;
  _senderThread->join();
  std::cout << "INFO: Join history thread" << std::endl;
}

int History::sendFront() {
  std::unique_lock<std::mutex> lock(_mutex);
  _condVar.wait_for(lock, std::chrono::milliseconds(200));
  if (_done) {
    return 1;
  }
  else {
    while (_histQueue.size() > 0) {
      send(_histQueue.front());
      _histQueue.pop_front();
    }
    return 0;
  }
}

int History::send(Message &message) {
  if (!enabled) {
    return 1;
  }

  int n = sendto(sock, &message, sizeof(Message), 0,
		 reinterpret_cast<struct sockaddr *>(&serveraddr), serverlen);
  if (n != sizeof(Message)) {
    perror("HistorySend");
    std::cerr << "ERROR: Failed to send history message (returned " << n << ")" << std::endl;
  }
  else {
    _counter++;
  }

  return 0;
}

void History::senderThread() {
  if (History::getInstance().enabled) {

    std::cout << "INFO: History sender thread ready." << std::endl;
    while (true) {
      if (History::getInstance().sendFront() > 0) {
	return;
      }
    }
  }
  return;
}
