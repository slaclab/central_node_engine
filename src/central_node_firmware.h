#ifndef CENTRAL_NODE_FIRMWARE_H
#define CENTRAL_NODE_FIRMWARE_H

#include <iostream>
#include <sstream>
#include <boost/shared_ptr.hpp>

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
const uint32_t FW_NUM_CONNECTIONS = 12;

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
  bool initialized;

 public:
  int loadConfig(std::string yamlFileName);

  friend class FirmwareTest;
  
 protected:
  Hub _root;
  Path _path;

  ScalVal_RO _fpgaVersionSV;
  ScalVal_RO _buildStampSV;
  ScalVal_RO _gitHashSV;
  ScalVal    _heartbeatSV;
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

  ScalVal_RO _rxPhyReadySV;
  ScalVal_RO _txPhyReadySV;
  ScalVal_RO _rxLocalLinkReadySV;
  ScalVal_RO _rxRemLinkReadySV;
  ScalVal_RO _rxFrameCountSV;
  ScalVal_RO _rxFrameErrorCountSV;

  uint8_t _heartbeat;

  void setBoolU64(ScalVal reg, bool enable);
  bool getBoolU64(ScalVal reg);
  uint32_t getUInt32(ScalVal_RO reg);
  uint8_t getUInt8(ScalVal_RO reg);

#ifndef FW_ENABLED
  int _updateSock;
  struct sockaddr_in clientaddr;
#endif

 public:
  uint64_t fpgaVersion;
  uint8_t buildStamp[256];
  char gitHashString[21];

  void heartbeat();

  void setEnable(bool enable);
  void setSoftwareEnable(bool enable);
  void setTimingCheckEnable(bool enable);
  void setEvaluationEnable(bool enable);
  void setTimeoutEnable(bool enable);

  bool getEnable();
  bool getSoftwareEnable();
  bool getTimingCheckEnable();
  bool getEvaluationEnable();
  bool getTimeoutEnable();
 
  void softwareClear();
  uint32_t getFaultReason();
  void showStats();
  
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
  void switchConfig();
  void evalLatchClear();
  void monErrClear();
  void swErrClear();
  void toErrClear();
  void moConcErrClear();

  uint64_t readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout);
  void writeMitigation(uint32_t *mitigation);
  void writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[]);

  static Firmware &getInstance() {
    static Firmware instance;
    return instance;
  }

  friend std::ostream & operator<<(std::ostream &os, Firmware * const firmware) {
    os << "=== MpsCentralNode ===" << std::endl;
    os << "FPGA version=" << firmware->fpgaVersion << std::endl;
    os << "Build stamp=\"" << firmware->buildStamp << "\"" << std::endl;
    os << "Git hash=\"" << firmware->gitHashString << "\"" << std::endl;

    try {
      os << "Enable=";
      if (firmware->getEnable()) os << "Enabled"; else os << "Disabled";
      os << std::endl;

      os << "SwEnable=";
      if (firmware->getSoftwareEnable()) os << "Enabled"; else os << "Disabled";
      os << std::endl;

      os << "SwLossCnt=" << firmware->getSoftwareLossCount() << std::endl;
      os << "SwLossError=" << (int) firmware->getSoftwareLossError() << std::endl;
      os << "SwBwidthCnt=" << firmware->getSoftwareClockCount() << std::endl;

      uint16_t aux[FW_NUM_BEAM_CLASSES]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      firmware->_beamIntTimeSV->getVal(aux, FW_NUM_BEAM_CLASSES);
      os << "BeamIntTime=[";
      for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i) {
	os << aux[i];
	if (i == FW_NUM_BEAM_CLASSES - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i) aux[i]=0;
      firmware->_beamMinPeriodSV->getVal(aux, FW_NUM_BEAM_CLASSES);
      os << "BeamMinPeriod=[";
      for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i) {
	os << aux[i];
	if (i == FW_NUM_BEAM_CLASSES - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i) aux[i]=0;
      firmware->_beamIntChargeSV->getVal(aux, FW_NUM_BEAM_CLASSES);
      os << "BeamIntCharge=[";
      for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i) {
	os << aux[i];
	if (i == FW_NUM_BEAM_CLASSES - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      os << "BeamFaultReason=" << std::hex << "0x"
	 << firmware->getUInt32(firmware->_beamFaultReasonSV) << std::dec << std::endl;

      os << "BeamFaultEnable=";
      if (firmware->getBoolU64(firmware->_beamFaultEnSV)) os << "Enabled"; else os << "Disabled";
      os << std::endl;

      uint8_t aux8[FW_NUM_MITIGATION_DEVICES]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      uint32_t aux32[2]={0,0};
      firmware->getMitigation(aux32);
      //      firmware->getMitigation(aux8);
      firmware->extractMitigation(aux32, aux8);
      os << "Mitigation=[";
      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) {
	os << (int) aux8[i];
	if (i == FW_NUM_MITIGATION_DEVICES - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      firmware->getFirmwareMitigation(aux32);
      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) aux8[i]=0;      
      //      firmware->getFirmwareMitigation(aux8);
      firmware->extractMitigation(aux32, aux8);
      os << "FirmwareMitigation=[";
      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) {
	os << (int) aux8[i];
	if (i == FW_NUM_MITIGATION_DEVICES - 1) os << "]"; else os << ", ";
      }
      os << std::endl;
      
      firmware->getSoftwareMitigation(aux32);
      os << "SoftwareMitigation=[";
      os << std::hex << "0x" << aux32[0] << " 0x" << aux32[1] << std::dec << "]" << std::endl;
      
      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) aux8[i]=0;      
      //      firmware->getSoftwareMitigation(aux8);
      firmware->extractMitigation(aux32, aux8);
      os << "SoftwareMitigation=[";
      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) {
	os << (int) aux8[i];
	if (i == FW_NUM_MITIGATION_DEVICES - 1) os << "]"; else os << ", ";
      }
      os << std::endl;
      
      firmware->getLatchedMitigation(aux32);
      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) aux8[i]=0;      
      firmware->extractMitigation(aux32, aux8);
      //      firmware->getLatchedMitigation(aux8);
      os << "LatchedMitigation=[";
      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) {
	os << (int) aux8[i];
	if (i == FW_NUM_MITIGATION_DEVICES - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      os << "MonitorReady=" << std::hex << "0x"
	 << firmware->getUInt32(firmware->_monitorReadySV) << std::dec << std::endl;

      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) aux[i]=0;      
      firmware->_monitorRxErrorCntSV->getVal(aux, FW_NUM_MITIGATION_DEVICES);
      os << "MonitorRxErrorCnt=[";
      for (uint32_t i = 0; i < FW_NUM_CONNECTIONS; ++i) {
	os << (int) aux[i];
	if (i == FW_NUM_CONNECTIONS - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) aux[i]=0;      
      firmware->_monitorPauseCntSV->getVal(aux, FW_NUM_MITIGATION_DEVICES);
      os << "MonitorPauseCnt=[";
      for (uint32_t i = 0; i < FW_NUM_CONNECTIONS; ++i) {
	os << (int) aux[i];
	if (i == FW_NUM_CONNECTIONS - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) aux[i]=0;      
      firmware->_monitorOvflCntSV->getVal(aux, FW_NUM_MITIGATION_DEVICES);
      os << "MonitorOvflCnt=[";
      for (uint32_t i = 0; i < FW_NUM_CONNECTIONS; ++i) {
	os << (int) aux[i];
	if (i == FW_NUM_CONNECTIONS - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      for (uint32_t i = 0; i < FW_NUM_MITIGATION_DEVICES; ++i) aux[i]=0;      
      firmware->_monitorDropCntSV->getVal(aux, FW_NUM_MITIGATION_DEVICES);
      os << "MonitorDropCnt=[";
      for (uint32_t i = 0; i < FW_NUM_CONNECTIONS; ++i) {
	os << (int) aux[i];
	if (i == FW_NUM_CONNECTIONS - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      os << "MonitorConcWdErrCnt="
	 << firmware->getUInt32(firmware->_monitorConcWdErrSV) << std::endl;

      os << "MonitorConcStallErrCnt="
	 << firmware->getUInt32(firmware->_monitorConcStallErrSV) << std::endl;

      os << "MonitorConcExtRxErrCnt="
	 << firmware->getUInt32(firmware->_monitorConcExtRxErrSV) << std::endl;

      os << "TimeoutErrStatus="
	 << firmware->getUInt32(firmware->_timeoutErrStatusSV) << std::endl;

      os << "TimeoutEnable=";
      if (firmware->getBoolU64(firmware->_timeoutEnableSV)) os << "Enabled"; else os << "Disabled";
      os << std::endl;

      os << "TimeoutTime="
	 << firmware->getUInt32(firmware->_timeoutTimeSV) << " usec"  << std::endl;

      os << "TimeoutMsgVer="
	 << firmware->getUInt32(firmware->_timeoutMsgVerSV) << std::endl;

      uint32_t aux32_32[32];
      for (uint32_t i = 0; i < 32; ++i) aux32_32[i]=0;      
      firmware->_timeoutErrIndexSV->getVal(aux32_32, 32);
      for (uint32_t i = 0; i < 32; ++i) {
	os << "0x" << std::hex << aux32_32[i] << std::dec;
	if (i == 32 - 1) os << "]"; else os << ", ";
      }
      os << std::endl;

      os << "EvaluationEnable=";
      if (firmware->getBoolU64(firmware->_evaluationEnableSV)) os << "Enabled"; else os << "Disabled";
      os << std::endl;

      os << "SoftwareWdTime="
	 << firmware->getUInt32(firmware->_swWdTimeSV) << std::endl;

      os << "SoftwareBusy="
	 << firmware->getUInt32(firmware->_swBusySV) << std::endl;

      os << "SoftwarePause="
	 << firmware->getUInt32(firmware->_swPauseSV) << std::endl;

      os << "SoftwareWdError="
	 << firmware->getUInt32(firmware->_swWdErrorSV) << std::endl;

      os << "SoftwareOvflCnt="
	 << firmware->getUInt32(firmware->_swOvflCntSV) << std::endl;

      os << "EvaluationTimeStamp="
	 << firmware->getUInt32(firmware->_evaluationTimeStampSV) << std::endl;

      os << "RxPhyReady="
	 << firmware->getUInt32(firmware->_rxPhyReadySV) << std::endl;

      os << "TxPhyReady="
	 << firmware->getUInt32(firmware->_txPhyReadySV) << std::endl;
      
      os << "RxLocalLinkReady="
	 << firmware->getUInt32(firmware->_rxLocalLinkReadySV) << std::endl;

      os << "RxRemLinkReady="
	 << firmware->getUInt32(firmware->_rxRemLinkReadySV) << std::endl;

      os << "RxFrameCount="
	 << firmware->getUInt32(firmware->_rxFrameCountSV) << std::endl;

      os << "RxFrameErrorCount="
	 << firmware->getUInt32(firmware->_rxFrameErrorCountSV) << std::endl;

    } catch (IOError &e) {
      std::cout << "Exception Info: " << e.getInfo() << std::endl;
    } catch (InvalidArgError &e) {
      std::cout << "Exception Info: " << e.getInfo() << std::endl;
    }

    /*(
  ScalVal    _timeoutClearSV;
  ScalVal    _timeoutMaskSV;
  ScalVal_RO _timeoutErrIndexSV;

  ScalVal    _evaluationClearSV;
  ScalVal_RO _evaluationTimeStampSV;
  ScalVal_RO _swOvflCntSV;
    */
    return os;
  }

};

typedef shared_ptr<Firmware> FirmwarePtr;

#endif
