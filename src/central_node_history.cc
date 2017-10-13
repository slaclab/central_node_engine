#include <central_node_history.h>
#include <central_node_exception.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include <netdb.h> 

History::History() : _counter(0), enabled (true) {
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

  int ret = pthread_mutex_init(&_mutex, NULL);
  if (0 != ret) {
    throw(CentralNodeException("ERROR: Failed to initialize mutex for MPS history sender."));
  }

  ret = pthread_cond_init(&_condition, NULL);
  if (0 != ret) {
    throw(CentralNodeException("ERROR: Failed to initialize condition variable for MPS history sender."));
  }

  if (enabled) {
    if (pthread_create(&_senderThread, 0, History::senderThread, 0)) {
      //      throw(CentralNodeException("ERROR: Failed to start message history sender thread"));
      std::cerr << "ERROR: Failed to start message history sender thread" 
		<< ", proceeding without MPS history." << std::endl;
      enabled = false;
    }
    else {
      std::cout << "INFO: MPS History sender ready" << std::endl;
    }
  }

}

int History::log(HistoryMessageType type, uint32_t id, uint32_t oldValue, uint32_t newValue, uint32_t aux) {
  message.id = id;
  message.oldValue = oldValue;
  message.newValue = newValue;
  message.type = type;
  message.aux = aux;

  return add(message);
}

int History::logFault(uint32_t id, uint32_t oldValue, uint32_t newValue, uint32_t faultStateId) {
  return log(FaultStateType, id, oldValue, newValue, faultStateId);
}

int History::logMitigation(uint32_t id, uint32_t oldValue, uint32_t newValue) {
  return log(MitigationType, id, oldValue, newValue, 0);
}

int History::logDeviceInput(uint32_t id, uint32_t oldValue, uint32_t newValue) {
  return log(DeviceInputType, id, oldValue, newValue, 0);
}

int History::logAnalogDevice(uint32_t id, uint32_t oldValue, uint32_t newValue) {
  return log(AnalogDeviceType, id, oldValue, newValue, 0);
}

int History::logBypassState(uint32_t id, uint32_t oldValue, uint32_t newValue, uint16_t index) {
  return log(BypassStateType, id, oldValue, newValue, index);
}

int History::logBypassValue(uint32_t id, uint32_t oldValue, uint32_t newValue) {
  return log(BypassValueType, id, oldValue, newValue, 0);
}

int History::add(Message &message) {
  if (!enabled) {
    return 1;
  }

  pthread_mutex_lock(&_mutex);
  if (_histQueue.size() >= HIST_QUEUE_MAX_SIZE) {
    pthread_mutex_unlock(&_mutex);
    return 1;
  }
  _histQueue.push_back(message);
  pthread_cond_signal(&_condition);
  pthread_mutex_unlock(&_mutex);

  return 0;
}

int History::sendFront() {
  pthread_mutex_lock(&_mutex);
  pthread_cond_wait(&_condition, &_mutex);
  while (_histQueue.size() > 0) {
    send(_histQueue.front());
    _histQueue.pop_front();
  }
  pthread_mutex_unlock(&_mutex);
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

void *History::senderThread(void *arg) {
  if (History::getInstance().enabled) {
    Message message;
    unsigned int priority;

    std::cout << "INFO: History sender thread ready." << std::endl;
    while (true) {
      History::getInstance().sendFront();
    }
  }
  return NULL;
}
