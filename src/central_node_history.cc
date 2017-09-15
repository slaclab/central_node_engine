#include <central_node_history.h>
#include <central_node_exception.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sstream>
#include <netdb.h> 

History::History() : enabled (true) {
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

  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = sizeof(Message); 
  attr.mq_curmsgs = 0;

  historyQueue = mq_open("/HistoryQueue", O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);

  if (historyQueue == -1) {
    std::stringstream errorStream;
    errorStream << "WARN: Failed to create MPS History Queue (errno="
		<< errno << ")";
    perror("HistoryQueue");
    enabled = false;
    std::cerr << errorStream.str() << ", proceeding without MPS history." << std::endl;
    //    throw(CentralNodeException(errorStream.str()));
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

  int size=sizeof(Message);// - sizeof(int32_t);
  int n = mq_send(historyQueue, reinterpret_cast<char *>(&message), size, 0);
  if (n != 0) {
    struct mq_attr attr;
    mq_getattr(historyQueue, &attr);
    perror("mq_send");
    std::cout << "INFO: max size is " << attr.mq_msgsize << ", msgsize is " << size << std::endl;
    return 1;
  }
  return 0;
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
  
  return 0;
}

void *History::senderThread(void *arg) {
  if (History::getInstance().enabled) {
    Message message;
    unsigned int priority;

    std::cout << "INFO: History sender thread ready." << std::endl;

    while (true) {
      int n = mq_receive(History::getInstance().historyQueue, reinterpret_cast<char *>(&message),
			 sizeof(Message), &priority);
      if (n == sizeof(Message)) {
	History::getInstance().send(message);
      }
      else {
	if (n == -1 && errno != EAGAIN) {
	  perror("mq_receive");
	}
	usleep(100);
	//sleep(1);
      }
    }
  }
  return NULL;
}
