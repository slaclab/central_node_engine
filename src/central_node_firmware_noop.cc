#include <central_node_firmware.h>
#include <central_node_exception.h>
#include <central_node_engine.h>
#include <stdint.h>
#include <log_wrapper.h>
#include <stdio.h>

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
using namespace easyloggingpp;
static Logger *firmwareLogger;
#endif

#ifndef FW_ENABLED

//====================================================================
// The following code is for testing the Central Node software
// without the Central Node board/firmware.
//====================================================================
#warning "Code compiled without CPSW - NO FIRMWARE"

#include <cstdlib>

//
// If firmware is not used an UDP socket server is created to receive
// simulated link node updates
//
Firmware::Firmware() :
  _updateCounter(0) {
  char *portString = getenv("CENTRAL_NODE_TEST_PORT");
  unsigned short port = 4356;
  if (portString != NULL) {
    port = atoi(portString);
    std::cout << "INFO: Server waiting on test data using port " << port << "." << std::endl;
  }


  _updateSock = socket(AF_INET, SOCK_DGRAM, 0);
  if (_updateSock < 0) {
    throw(CentralNodeException("ERROR: Failed to open socket for simulated firmware inputs"));
  }

  int optval = 1;
  setsockopt(_updateSock, SOL_SOCKET, SO_REUSEADDR,
             (const void *) &optval , sizeof(int));

  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  setsockopt(_updateSock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  struct sockaddr_in serveraddr;
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(port);

  if (bind(_updateSock, (struct sockaddr *) &serveraddr,
           sizeof(serveraddr)) < 0) {
    throw(CentralNodeException("ERROR: Failed to open socket for simulated firmaware inputs"));
  }

  fpgaVersion = 1;
  for (int i = 0; i < 256; ++i) {
    buildStamp[i] = 0;
  }
  sprintf(gitHashString, "NONE");

  std::cout <<  ">>> Code compiled without CPSW - NO FIRMWARE <<<" << std::endl;
};

Firmware::~Firmware() {
}

int Firmware::createRoot(std::string yamlFileName) {
  return 0;
}

Path Firmware::getRoot() {
  return _root;
}

void Firmware::setRoot(Path root) {
}

int Firmware::createRegisters() {
  return 0;
}

void Firmware::writeAppTimeoutMask() {
}

void Firmware::setAppTimeoutEnable(uint32_t appId, bool enable) {
}

void Firmware::getAppTimeoutStatus() {
}

bool Firmware::getAppTimeoutStatus(uint32_t appId) {
  return false;
}

void Firmware::setBoolU64(ScalVal reg, bool enable) {
}

bool Firmware::getBoolU64(ScalVal reg) {
  return false;
}

uint64_t Firmware::getUInt64(ScalVal_RO reg) {
  return 0;
}

uint32_t Firmware::getUInt32(ScalVal_RO reg) {
  return 0;
}

uint32_t Firmware::getUInt32(ScalVal reg) {
  return 0;
}

uint8_t Firmware::getUInt8(ScalVal_RO reg) {
  return 0;
}

bool Firmware::getEnable() {
  return false;
}

uint32_t Firmware::getSoftwareClockCount() {
  return 0;
}

uint8_t Firmware::getSoftwareLossError() {
  return 0;
}

uint32_t Firmware::getSoftwareLossCount() {
  return 0;
}

void Firmware::setEvaluationEnable(bool enable) {
}

bool Firmware::getEvaluationEnable() {
  return false;
}

void Firmware::setTimeoutEnable(bool enable) {
}

bool Firmware::getTimeoutEnable() {
  return 0;
}

void Firmware::setTimingCheckEnable(bool enable) {
}

bool Firmware::getTimingCheckEnable() {
  return false;
}

uint32_t Firmware::getFaultReason() {
  return 0;
}

void Firmware::getFirmwareMitigation(uint32_t *fwMitigation) {
  fwMitigation[0] = 0;
  fwMitigation[1] = 0;
}

void Firmware::getSoftwareMitigation(uint32_t *swMitigation) {
  swMitigation[0] = 0;
  swMitigation[1] = 0;
}

void Firmware::getMitigation(uint32_t *mitigation) {
  mitigation[0] = 0;
  mitigation[1] = 0;
}

void Firmware::getLatchedMitigation(uint32_t *latchedMitigation) {
  latchedMitigation[0] = 0;
  latchedMitigation[1] = 0;
}

void Firmware::extractMitigation(uint32_t *compressed, uint8_t *expanded) {
  for (int i = 0; i < 16; ++i) {
    expanded[i] = 0;
  }
}

bool Firmware::heartbeat() {
  return false;
}

bool Firmware::evalLatchClear() {
  return false;
}

bool Firmware::monErrClear() {
  return false;
}

bool Firmware::swErrClear() {
  return false;
}

bool Firmware::toErrClear() {
  return false;
}

bool Firmware::moConcErrClear() {
  return false;
}

bool Firmware::clearAll() {
  return false;
}

std::ostream & operator<<(std::ostream &os, Firmware * const firmware) {
    os << "=== MpsCentralNode ===" << std::endl;
    os << "FPGA version=" << firmware->fpgaVersion << std::endl;
    os << "Build stamp=\"" << firmware->buildStamp << "\"" << std::endl;
    os << "Git hash=\"" << firmware->gitHashString << "\"" << std::endl;
    os << "Updates received=" << firmware->_updateCounter << std::endl;
}

bool Firmware::switchConfig() {
  return false;
}

void Firmware::setEnable(bool enable) {
}

void Firmware::setSoftwareEnable(bool enable) {
}

bool Firmware::getSoftwareEnable() {
  return false;
}

void Firmware::writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size) {
};

void Firmware::softwareClear() {
};

void Firmware::writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[]) {
};

void Firmware::showStats() {
}

typedef struct {
  char source[128];
  char date[64];
  char user[64];
  char md5sum[64];
} DatabaseInfo;

uint64_t Firmware::readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout) {
  socklen_t clientlen = sizeof(clientaddr);

  std::cout << "INFO: waiting for simulated data..." << std::endl;
  int n = recvfrom(_updateSock, buffer, size, 0,
		   (struct sockaddr *) &clientaddr, &clientlen);
  if (n < 0) {
    std::cerr << "ERROR: Failed to receive simulated firmware data" << std::endl;
    return 0;
  }
  else {
    std::cout << "INFO: Received " << n << " bytes." <<std::endl;
  }

  if (buffer[0] == 'I' &&
      buffer[1] == 'N' &&
      buffer[2] == 'F' &&
      buffer[3] == 'O' &&
      buffer[4] == '?') {

    std::cout << "INFO: Received request for database info" << std::endl;

    socklen_t clientlen = sizeof(clientaddr);

    DatabaseInfo info;
    strncpy(info.source, Engine::getInstance().getCurrentDb()->databaseInfo->at(0)->source.c_str(), 128);
    strncpy(info.date, Engine::getInstance().getCurrentDb()->databaseInfo->at(0)->date.c_str(), 64);
    strncpy(info.user, Engine::getInstance().getCurrentDb()->databaseInfo->at(0)->user.c_str(), 64);
    strncpy(info.md5sum, Engine::getInstance().getCurrentDb()->databaseInfo->at(0)->md5sum.c_str(), 64);

    int n = sendto(_updateSock, &info, sizeof(DatabaseInfo), 0,
		   reinterpret_cast<struct sockaddr *>(&clientaddr), clientlen);

    return 0;
  }

  _updateCounter++;

  return n;
};

void Firmware::writeMitigation(uint32_t *mitigation) {
  uint32_t swapMitigation[2];
  swapMitigation[0] = mitigation[1];
  swapMitigation[1] = mitigation[0];
  socklen_t clientlen = sizeof(clientaddr);
  /*
  for (int i = 0; i < 2; ++i) {
    uint32_t powerClass = mitigation[i];
    for (int j = 0; j < 32; j = j + 4) {
      std::cout << "Destination[" << i * 8 + j/4<< "]: Class[" << ((powerClass >> j) & 0xF) << "]" << std::endl;
    }
  }
  */
  int n = sendto(_updateSock, &swapMitigation, 2*sizeof(uint32_t), 0, reinterpret_cast<struct sockaddr *>(&clientaddr), clientlen);
}

#endif
