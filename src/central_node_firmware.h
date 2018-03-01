#ifndef CENTRAL_NODE_FIRMWARE_H
#define CENTRAL_NODE_FIRMWARE_H

#include <iostream>
#include <sstream>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include <time_util.h>

#include <cpsw_api_user.h>

#ifndef FW_ENABLED
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using boost::shared_ptr;

const uint32_t FW_NUM_APPLICATIONS = 8;
const uint32_t FW_NUM_BEAM_CLASSES = 16;
const uint32_t FW_NUM_MITIGATION_DEVICES = 16;
const uint32_t FW_NUM_BEAM_DESTINATIONS = 16;
const uint32_t FW_NUM_CONNECTIONS = 12;
const uint32_t FW_NUM_APPLICATION_MASKS = 1024;
const uint32_t FW_NUM_APPLICATION_MASKS_WORDS = FW_NUM_APPLICATION_MASKS/sizeof(uint32_t);

typedef std::bitset<FW_NUM_APPLICATION_MASKS> ApplicationBitMaskSet;

// TODO: There is a status register, beamFaultReason. Bits 15:0 indicate a power
// class violation for each destination. Bits 31:16 indicate a min period
// violation for each period. Faults are latched and are cleared with the
// beamFaultClr register.

// TODO: add timing info registers (see TimingFrameRx.yaml file)

/**
 */
class Firmware {
 private:
  Firmware();
  Firmware(Firmware const &);
  ~Firmware();

  void operator=(Firmware const &);
  uint32_t _applicationTimeoutMaskBuffer[FW_NUM_APPLICATION_MASKS_WORDS];
  ApplicationBitMaskSet *_applicationTimeoutMaskBitSet;

  uint32_t _applicationTimeoutErrorBuffer[FW_NUM_APPLICATION_MASKS_WORDS];
  ApplicationBitMaskSet *_applicationTimeoutErrorBitSet;

 public:
  int createRoot(std::string yamlFileName);
  void setRoot(Path root);
  int createRegisters();
  Path getRoot();

  friend class FirmwareTest;
  
 protected:
  Path _root;

  ScalVal_RO _fpgaVersionSV;
  ScalVal_RO _buildStampSV;
  ScalVal_RO _gitHashSV;
  ScalVal    _enableSV;
  ScalVal_RO _swLossErrorSV;
  ScalVal_RO _swLossCntSV;
  ScalVal_RO _txClkCntSV; 
  ScalVal    _configSV[FW_NUM_APPLICATIONS];
  ScalVal    _swEnableSV;
  ScalVal    _swClearSV;
  ScalVal    _beamIntTimeSV;
  ScalVal    _beamMinPeriodSV;
  ScalVal    _beamIntChargeSV;
  ScalVal_RO _beamFaultReasonSV;
  ScalVal    _beamFaultEnSV;
  ScalVal    _beamFaultClrSV;
  ScalVal    _swMitigationSV;
  ScalVal_RO _fwMitigationSV;
  ScalVal_RO _mitigationSV;
  ScalVal_RO _latchedMitigationSV;
  ScalVal_RO _monitorReadySV;
  ScalVal_RO _monitorRxErrorCntSV;
  ScalVal_RO _monitorPauseCntSV;
  ScalVal_RO _monitorOvflCntSV;
  ScalVal_RO _monitorDropCntSV;
  ScalVal_RO _monitorConcWdErrSV;
  ScalVal_RO _monitorConcStallErrSV;
  ScalVal_RO _monitorConcExtRxErrSV;
  ScalVal    _timeoutEnableSV;
  ScalVal    _timeoutClearSV;
  ScalVal    _timeoutTimeSV;
  ScalVal    _timeoutMaskSV;
  ScalVal_RO _timeoutErrStatusSV;
  ScalVal_RO _timeoutErrIndexSV;
  ScalVal    _timeoutMsgVerSV;
  ScalVal    _evaluationEnableSV;
  ScalVal_RO _evaluationTimeStampSV;
  ScalVal    _swWdTimeSV;
  ScalVal_RO _swBusySV;
  ScalVal_RO _swPauseSV;
  ScalVal_RO _swWdErrorSV;
  ScalVal_RO _swOvflCntSV;
  Stream     _updateStreamSV;

  Command    _swHeartbeatCmd;
  Command    _monErrClearCmd;
  Command    _swErrClearCmd;
  Command    _switchConfigCmd;
  Command    _evalLatchClearCmd;
  Command    _toErrClearCmd;
  Command    _moConcErrClearCmd;

  void setBoolU64(ScalVal reg, bool enable);
  bool getBoolU64(ScalVal reg);
  uint64_t getUInt64(ScalVal_RO reg);
  uint32_t getUInt32(ScalVal_RO reg);
  uint32_t getUInt32(ScalVal reg);
  uint8_t getUInt8(ScalVal_RO reg);

#ifndef FW_ENABLED
  int _updateSock;
  struct sockaddr_in clientaddr;
  int _updateCounter;
#endif

 public:
  uint64_t fpgaVersion;
  uint8_t buildStamp[256];
  char gitHashString[21];

  bool _firstHeartbeat;
  TimeAverage _heartbeatTime;

  static uint32_t _swLossErrorCounter;
  static uint32_t _swBusyCounter;
  static uint32_t _swPauseCounter;
  static uint32_t _swWdErrorCounter;
  static uint32_t _swOvflCntCounter;
  static uint32_t _monitorNotReadyCounter;

  static bool _skipHeartbeat;

  bool heartbeat();

  void setEnable(bool enable);
  void setSoftwareEnable(bool enable);
  void setTimingCheckEnable(bool enable);
  void setEvaluationEnable(bool enable);
  void setTimeoutEnable(bool enable);
  void setAppTimeoutEnable(uint32_t appId, bool enable);
  bool getAppTimeoutStatus(uint32_t appId);

  bool getEnable();
  bool getSoftwareEnable();
  bool getTimingCheckEnable();
  bool getEvaluationEnable();
  bool getTimeoutEnable();
 
  void softwareClear();
  uint32_t getFaultReason();
  void showStats();

  void writeAppTimeoutMask();
  void getAppTimeoutStatus();

  uint8_t getSoftwareLossError();
  uint32_t getSoftwareLossCount();
  uint32_t getSoftwareClockCount();
  void getFirmwareMitigation(uint32_t *fwMitigation);
  void getSoftwareMitigation(uint32_t *swMitigation);
  void getMitigation(uint32_t *mitigation);
  void getLatchedMitigation(uint32_t *latchedMitigation);
  void extractMitigation(uint32_t *compressed, uint8_t *expanded);

  // size in bytes (not in uint32_t units)
  void writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size);
  bool switchConfig();

  bool evalLatchClear();
  bool monErrClear();
  bool swErrClear();
  bool toErrClear();
  bool moConcErrClear();
  bool clearAll();

  uint64_t readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout);
  void writeMitigation(uint32_t *mitigation);
  void writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[]);
  
  static Firmware &getInstance() {
    static Firmware instance;
    return instance;
  }

  friend std::ostream & operator<<(std::ostream &os, Firmware * const firmware);
};

typedef shared_ptr<Firmware> FirmwarePtr;

#endif
