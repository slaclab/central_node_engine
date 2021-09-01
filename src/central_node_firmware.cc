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

// uint32_t Firmware::_swWdErrorCounter = 0;
// uint32_t Firmware::_swLossErrorCounter = 0;
// uint32_t Firmware::_swBusyCounter = 0;
// uint32_t Firmware::_swPauseCounter = 0;
// uint32_t Firmware::_swOvflCntCounter = 0;
// uint32_t Firmware::_monitorNotReadyCounter = 0;
// bool     Firmware::_skipHeartbeat = false;

// These are the labels for the bit in the flag word, in the power
// class asynchronous message (defined in the pc_counter_t structure)
// Note: the method that print them, assumens that the longer string
// is 12 char long.
const std::vector<std::string> Firmware::PcChangePacketFlagsLabels =
{
    "MonReady",     // Bit  0
    "ExtRxErr",     // Bit  1
    "RxErr",        // Bit  2
    "Pause",        // Bit  3
    "Ovfl",         // Bit  4
    "Drop",         // Bit  5
    "ConWdErr2",    // Bit  6
    "ConWdErr1",    // Bit  7
    "ConWdErr0",    // Bit  8
    "ConStallErr2", // Bit  9
    "ConStallErr1", // Bit 10
    "ConStallErr0", // Bit 11
    "TimeoutErr",   // Bit 12
    "SwErr",        // Bit 13
    "Enables"       // Bit 14
    // Bit 15 is not used at the moment
};

#ifdef FW_ENABLED
Firmware::Firmware()
{
#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
    firmwareLogger = Loggers::getLogger("FIRMWARE");
    LOG_TRACE("FIRMWARE", "Created Firmware");
#endif
    for (uint32_t i = 0; i < FW_NUM_APPLICATION_MASKS_WORDS; ++i)
    {
        _applicationTimeoutMaskBuffer[i] = 0;
        _applicationTimeoutErrorBuffer[i] = 0;
    }
    _applicationTimeoutMaskBitSet = reinterpret_cast<ApplicationBitMaskSet *>(_applicationTimeoutMaskBuffer);
    _applicationTimeoutErrorBitSet = reinterpret_cast<ApplicationBitMaskSet *>(_applicationTimeoutErrorBuffer);
}

Firmware::~Firmware()
{
    setSoftwareEnable(false);
    setEnable(false);
}

int Firmware::createRoot(std::string yamlFileName)
{
    Hub hub = IPath::loadYamlFile(yamlFileName.c_str(), "NetIODev")->origin();
    _root = IPath::create(hub);
    return 0;
}

Path Firmware::getRoot()
{
    return _root;
}

void Firmware::setRoot(Path root)
{
    _root = root;
}

// TODO: separate yaml loading and scalval creation into two funcs - so can call yamlloader
int Firmware::createRegisters()
{
    uint8_t gitHash[21];

    if (!_root)
        return 1;

    std::string base = "/mmio";
    std::string axi = "/AmcCarrierCore/AxiVersion";
    std::string mps = "/MpsCentralApplication";
    std::string core = "/MpsCentralNodeCore";
    std::string config = "/MpsCentralNodeConfig";

    std::string name;

    try
    {
        name = base + axi + "/FpgaVersion";
        _fpgaVersionSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + axi + "/BuildStamp";
        _buildStampSV  = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + axi + "/GitHash";
        _gitHashSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/SwHeartbeat";
        _swHeartbeatCmd = ICommand::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/EvalLatchClear";
        _evalLatchClearCmd = ICommand::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/Enable";
        _enableSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/SoftwareEnable";
        _swEnableSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/SoftwareClear";
        _swClearSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/SoftwareLossError";
        _swLossErrorSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/SoftwareLossCnt";
        _swLossCntSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/SoftwareBwidthCnt";
        _txClkCntSV = IScalVal_RO::create (_root->findByName(name.c_str()));

        name = base + mps + core  + "/BeamIntTime";
        _beamIntTimeSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/BeamMinPeriod";
        _beamMinPeriodSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/BeamIntCharge";
        _beamIntChargeSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/BeamFaultReason";
        _beamFaultReasonSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/BeamFaultEn";
        _beamFaultEnSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/EvalLatchClear";
        _evalLatchClearCmd = ICommand::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/MonErrClear";
        _monErrClearCmd = ICommand::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/SwErrClear";
        _swErrClearCmd = ICommand::create(_root->findByName(name.c_str()));

        name = base + mps + core  + "/BeamFaultClr";
        _beamFaultClrSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/EvaluationSwPowerLevel";
        _swMitigationSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/EvaluationFwPowerLevel";
        _fwMitigationSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/EvaluationLatchedPowerLevel";
        _latchedMitigationSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/EvaluationPowerLevel";
        _mitigationSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/SwitchConfig";
        _switchConfigCmd = ICommand::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/ToErrClear";
        _toErrClearCmd = ICommand::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MoConcErrClear";
        _moConcErrClearCmd = ICommand::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MonitorReady";
        _monitorReadySV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MonitorRxErrCnt";
        _monitorRxErrorCntSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MonitorPauseCnt";
        _monitorPauseCntSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MonitorOvflCnt";
        _monitorOvflCntSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MonitorDropCnt";
        _monitorDropCntSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MonitorConcWdErr";
        _monitorConcWdErrSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MonitorConcStallErr";
        _monitorConcStallErrSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/MonitorConcExtRxErr";
        _monitorConcExtRxErrSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/TimeoutEnable";
        _timeoutEnableSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/TimeoutClear";
        _timeoutClearSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/TimeoutTime";
        _timeoutTimeSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/TimeoutMask";
        _timeoutMaskSV = IScalVal::create(_root->findByName(name.c_str()));

        // Initialize timeout mask to zero
        writeAppTimeoutMask();

        name = base + mps + core + "/TimeoutErrStatus";
        _timeoutErrStatusSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/TimeoutErrIndex";
        _timeoutErrIndexSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/TimeoutMsgVer";
        _timeoutMsgVerSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/EvaluationEnable";
        _evaluationEnableSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/EvaluationTimeStamp";
        _evaluationTimeStampSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/SoftwareWdTime";
        _swWdTimeSV = IScalVal::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/SoftwareBusy";
        _swBusySV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/SoftwarePause";
        _swPauseSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/SoftwareWdError";
        _swWdErrorSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = base + mps + core + "/SoftwareOvflCnt";
        _swOvflCntSV = IScalVal_RO::create(_root->findByName(name.c_str()));

        name = "/Stream0";
        _updateStreamSV = IStream::create(_root->findByName(name.c_str()));

        name = "/Stream1";
        _pcChangeStreamSV = IStream::create(_root->findByName(name.c_str()));

        // ScalVal for configuration
        for (uint32_t i = 0; i < FW_NUM_APPLICATIONS; ++i)
        {
            std::stringstream appId;
            appId << "/AppId[" << i << "]/Config";
            name = base + mps + config + appId.str();
            _configSV[i] = IScalVal::create(_root->findByName(name.c_str()));
        }
    }
    catch (NotFoundError &e)
    {
        throw(CentralNodeException("ERROR: Failed to find " + name));
    }
    catch (InterfaceNotImplementedError &e)
    {
        throw(CentralNodeException("ERROR: Wrong interface for " + name));
    }

    if (_beamIntTimeSV->getNelms() != FW_NUM_BEAM_CLASSES)
        throw(CentralNodeException("ERROR: Invalid number of beam classes (BeamIntTime)"));
    if (_beamMinPeriodSV->getNelms() != FW_NUM_BEAM_CLASSES)
        throw(CentralNodeException("ERROR: Invalid number of beam classes (BeamMinPeriod)"));
    if (_beamIntChargeSV->getNelms() != FW_NUM_BEAM_CLASSES)
        throw(CentralNodeException("ERROR: Invalid number of beam classes (BeamIntCharge)"));

    // Read some registers that are static
    try
    {
        _fpgaVersionSV->getVal(&fpgaVersion);
        _buildStampSV->getVal(buildStamp, sizeof(buildStamp)/sizeof(buildStamp[0]));
        _gitHashSV->getVal(gitHash, sizeof(gitHash)/sizeof(gitHash[0]));
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on createRegisters()" << std::endl;
    }

    for (int i = 0; i < 20; i++)
        sprintf(&gitHashString[i], "%X", gitHash[i]);
    gitHashString[20] = 0;

    return 0;
}

void Firmware::writeAppTimeoutMask()
{
    try
    {
        _timeoutMaskSV->setVal(&_applicationTimeoutMaskBuffer[0], FW_NUM_APPLICATION_MASKS_WORDS);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on writeAppTimeoutMask()" << std::endl;
    }
}

void Firmware::setAppTimeoutEnable(uint32_t appId, bool enable, bool writeFW)
{
    // Ignore invalid App IDs
    if (appId >= FW_NUM_APPLICATION_MASKS)
        return;

    _applicationTimeoutMaskBitSet->set(appId, enable);

    // Write the configuration to FW
    if (writeFW)
        writeAppTimeoutMask();
}

bool Firmware::getAppTimeoutEnable(uint32_t appId)
{
    return _applicationTimeoutMaskBitSet->test(appId);
}

void Firmware::getAppTimeoutStatus()
{
    try
    {
        _timeoutErrIndexSV->getVal(&_applicationTimeoutErrorBuffer[0], FW_NUM_APPLICATION_MASKS_WORDS);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getAppTimeoutStatus()" << std::endl;
    }
}

// @return true (bit is set) if there is timeout error, false if not (bit reset)
bool Firmware::getAppTimeoutStatus(uint32_t appId)
{
    if (appId < FW_NUM_APPLICATION_MASKS)
        return (*_applicationTimeoutErrorBitSet)[appId];

    return true;
}

void Firmware::setBoolU64(ScalVal reg, bool enable)
{
    try
    {
        uint64_t value = 0;
        if (enable) value = 1;
        reg->setVal(value);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error setBoolU64(reg, bool)" << std::endl;
    }
}

bool Firmware::getBoolU64(ScalVal reg)
{
    uint64_t value;
    try
    {
        reg->getVal(&value);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getBoolU64(reg)" << std::endl;
        return false;
    }

    if (value == 0)
        return false;
    else
        return true;
}

uint64_t Firmware::getUInt64(ScalVal_RO reg)
{
    uint64_t value = 0;
    try
    {
        reg->getVal(&value);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getUInt64(reg)" << std::endl;
    }
    return value;
}

uint32_t Firmware::getUInt32(ScalVal_RO reg)
{
    uint32_t value = 0;
    try
    {
        reg->getVal(&value);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getUInt32(reg)" << std::endl;
    }
    return value;
}

uint32_t Firmware::getUInt32(ScalVal reg)
{
    uint32_t value = 0;
    try
    {
        reg->getVal(&value);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getUInt32(reg)" << std::endl;
    }
    return value;
}

uint8_t Firmware::getUInt8(ScalVal_RO reg)
{
    uint8_t value = 0;
    try
    {
        reg->getVal(&value);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getUInt8(reg)" << std::endl;
    }
    return value;
}

void Firmware::setEnable(bool enable)
{
    setBoolU64(_enableSV, enable);
}

bool Firmware::getEnable()
{
    return getBoolU64(_enableSV);
}

uint32_t Firmware::getSoftwareClockCount()
{
    return getUInt32(_txClkCntSV);
}

uint8_t Firmware::getSoftwareLossError()
{
    return getUInt8(_swLossErrorSV);
}

uint32_t Firmware::getSoftwareLossCount()
{
    return getUInt32(_swLossCntSV);
}

void Firmware::setEvaluationEnable(bool enable)
{
    setBoolU64(_evaluationEnableSV, enable);
}

bool Firmware::getEvaluationEnable()
{
    return getBoolU64(_evaluationEnableSV);
}

void Firmware::setTimeoutEnable(bool enable)
{
    setBoolU64(_timeoutEnableSV, enable);
}

bool Firmware::getTimeoutEnable() {
  return getBoolU64(_timeoutEnableSV);
}

void Firmware::setTimingCheckEnable(bool enable)
{
    setBoolU64(_beamFaultEnSV, enable);
}

void Firmware::setSoftwareEnable(bool enable)
{
    setBoolU64(_swEnableSV, enable);
}

bool Firmware::getSoftwareEnable()
{
    return getBoolU64(_swEnableSV);
}

bool Firmware::getTimingCheckEnable()
{
    return getBoolU64(_beamFaultEnSV);
}

void Firmware::softwareClear()
{
    setBoolU64(_swClearSV, true);
    setBoolU64(_swClearSV, false);
}

uint32_t Firmware::getFaultReason()
{
    return getUInt32(_beamFaultReasonSV);
}

void Firmware::getFirmwareMitigation(uint32_t *fwMitigation)
{
    try
    {
        _fwMitigationSV->getVal(fwMitigation, 2);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getFirmwareMitigation()" << std::endl;
    }
}

void Firmware::getSoftwareMitigation(uint32_t *swMitigation)
{
    try
    {
        _swMitigationSV->getVal(swMitigation, 2);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getSoftwareMitigation()" << std::endl;
    }
}

void Firmware::getMitigation(uint32_t *mitigation)
{
    try
    {
        _mitigationSV->getVal(mitigation, 2);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getMitigation()" << std::endl;
    }
}

void Firmware::getLatchedMitigation(uint32_t *latchedMitigation)
{
    try
    {
        _latchedMitigationSV->getVal(latchedMitigation, 2);
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on getLatchedMitigation()" << std::endl;
    }
}

void Firmware::extractMitigation(uint32_t *compressed, uint8_t *expanded)
{
    for (int i = 0; i < 8; ++i)
        expanded[i] = (uint8_t)((compressed[0] >> (4 * i)) & 0xF);

    for (int i = 0; i < 8; ++i)
        expanded[i+8] = (uint8_t)((compressed[1] >> (4 * i)) & 0xF);
}

void Firmware::writeConfig(uint32_t appNumber, uint8_t *config, uint32_t size)
{
    uint32_t *config32 = reinterpret_cast<uint32_t *>(config);
    uint32_t size32 = size / 4;

    if (appNumber < FW_NUM_APPLICATIONS)
    {
        try
        {
            LOG_TRACE("FIRMWARE", "Writing configuration for application number #" << appNumber << " data size=" << size32);
            _configSV[appNumber]->setVal(config32, size32);
        }
        catch (InvalidArgError &e)
        {
            throw(CentralNodeException("ERROR: Failed writing app configuration (InvalidArgError)."));
        }
        catch (BadStatusError &e)
        {
            std::cout << "Exception Info: " << e.getInfo() << std::endl;
            throw(CentralNodeException("ERROR: Failed writing app configuration (BadStatusError)"));
        }
        catch (IOError &e)
        {
            std::cout << "Exception Info: " << e.getInfo() << std::endl;
            showStats();
            throw(CentralNodeException("ERROR: Failed writing app configuration (IOError)"));
        }
    }

    // Hit the switch register to enable now configuration
    //  switchConfig();
}

bool Firmware::switchConfig()
{
    try
    {
        _switchConfigCmd->execute();
    }
    catch (IOError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
        return false;
    }
    return true;
}

bool Firmware::evalLatchClear()
{
    try
    {
        _evalLatchClearCmd->execute();
    }
    catch (IOError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
        return false;
    }
    return true;
}

bool Firmware::monErrClear()
{
    try
    {
        _monErrClearCmd->execute();
    }
    catch (IOError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
        return false;
    }
    return true;
}

bool Firmware::swErrClear()
{
    try
    {
        _swErrClearCmd->execute();
    }
    catch (IOError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
        return false;
    }
    return true;
}

bool Firmware::toErrClear()
{
    try
    {
        _toErrClearCmd->execute();
    }
    catch (IOError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
        return false;
    }
    return true;
}

bool Firmware::moConcErrClear()
{
    try
    {
        _moConcErrClearCmd->execute();
    }
    catch (IOError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
        return false;
    }
    return true;
}

bool Firmware::beamFaultClear()
{
    try
    {
        uint64_t val = 1;
        _beamFaultClrSV->setVal(val);
        val = 0;
        _beamFaultClrSV->setVal(val);
    }
    catch (IOError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
        return false;
    }
    return true;
}

bool Firmware::clearAll()
{
    return evalLatchClear() ||
        monErrClear() ||
        swErrClear() ||
        toErrClear() ||
        beamFaultClear() ||
        moConcErrClear();
}

/**
 * This is for the integration time (input time array)
 * intBits[25:16] = (period>1000)?((period/1000)-1):(0)
 * intBits[9:0] = (period > 1000)?999:(period-1)
 *
 * PC1: 100,000 will be written as (100 milliseconds)
 *      999 in the lower bits
 *       99 in the upper bits
 *
 * PC2: 1,000,000 will be written as (1 second)
 *      999 in lower bits
 *      999 in the upper bits
 */
void Firmware::writeTimingChecking(uint32_t time[], uint32_t period[], uint32_t charge[])
{
    try
    {
        _beamMinPeriodSV->setVal(period, FW_NUM_BEAM_CLASSES);
        _beamIntChargeSV->setVal(charge, FW_NUM_BEAM_CLASSES);

        uint32_t new_time[FW_NUM_BEAM_CLASSES];
        for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i)
        {
            new_time[i] = 0;
            if (time[i] > 1000)
            {
                // Set bits 25:16 first
                new_time[i] = time[i]/1000 - 1;
                new_time[i] <<= 16;
                // Set lower bits next
                new_time[i] |= 999;
            }
            else
            {
                if (time[i] != 0)
                    new_time[i] = time[i];
            }
        }

        _beamIntTimeSV->setVal(new_time, FW_NUM_BEAM_CLASSES);

    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on writeTimingChecking()" << std::endl;
    }
}

uint64_t Firmware::readUpdateStream(uint8_t *buffer, uint32_t size, uint64_t timeout)
{
    uint64_t val = 0;
    try
    {
        val = _updateStreamSV->read(buffer, size, CTimeout(timeout));
    }
    catch (IOError &e)
    {
        std::cout << "Failed to read update stream exception: " << e.getInfo() << std::endl;
        val = 0;
    }
    return val;
}

int64_t Firmware::readPCChangeStream(uint8_t *buffer, uint32_t size, uint64_t timeout) const
{
    int64_t ret {0};

    try
    {
        ret =  _pcChangeStreamSV->read(buffer, size, CTimeout(timeout));
    }
    catch (CPSWError& e)
    {
       std::cerr << "Failed to read the power class change stream exception: " << e.getInfo() << std::endl;
       ret = 0;
    }

    return ret;
}

void Firmware::writeMitigation(const std::vector<uint32_t>& mitigation)
{
    //DEBUG
    //  mitigation[0] = 0x55667788;
    //  mitigation[1] = 0x11223344;
    //DEBUG
    try
    {
        _swMitigationSV->setVal(const_cast<uint32_t*>(mitigation.data()), mitigation.size());
    }
    catch (IOError &e)
    {
        std::cerr << "ERROR: CPSW I/O Error on writeMitigation()" << std::endl;
    }
}

void Firmware::showStats()
{
    Children rootsChildren = _root->origin()->getChildren();
    std::vector<Child>::const_iterator it;

    for ( it = rootsChildren->begin(); it != rootsChildren->end(); ++it )
        (*it)->dump();
}

std::ostream & operator<<(std::ostream &os, Firmware * const firmware)
{
    os << "=== MpsCentralNode ===" << std::endl;
    os << "FPGA version=" << firmware->fpgaVersion << std::endl;
    os << "Build stamp=\"" << firmware->buildStamp << "\"" << std::endl;
    os << "Git hash=\"" << firmware->gitHashString << "\"" << std::endl;

    try
    {
        os << "Enable=";
        if (firmware->getEnable()) os << "Enabled"; else os << "Disabled";
            os << std::endl;

        os << "SwEnable=";
        if (firmware->getSoftwareEnable()) os << "Enabled"; else os << "Disabled";
            os << std::endl;

        os << "SwLossCnt=" << firmware->getSoftwareLossCount() << std::endl;
        os << "SwLossError=" << (int) firmware->getSoftwareLossError() << std::endl;
        os << "SwBwidthCnt=" << firmware->getSoftwareClockCount() << std::endl;

        uint32_t aux[FW_NUM_BEAM_CLASSES]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        firmware->_beamIntTimeSV->getVal(aux, FW_NUM_BEAM_CLASSES);
        os << "BeamIntTime=[";
        for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i)
        {
            os << aux[i];
            if (i == FW_NUM_BEAM_CLASSES - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i)
            aux[i]=0;
        firmware->_beamMinPeriodSV->getVal(aux, FW_NUM_BEAM_CLASSES);
        os << "BeamMinPeriod=[";
        for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i)
        {
            os << aux[i];
            if (i == FW_NUM_BEAM_CLASSES - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i)
            aux[i]=0;
        firmware->_beamIntChargeSV->getVal(aux, FW_NUM_BEAM_CLASSES);
        os << "BeamIntCharge=[";
        for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i)
        {
            os << aux[i];
            if (i == FW_NUM_BEAM_CLASSES - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        os << "BeamFaultReason=" << std::hex << "0x"
            << firmware->getUInt32(firmware->_beamFaultReasonSV) << std::dec << std::endl;

        os << "BeamFaultEnable=";
        if (firmware->getBoolU64(firmware->_beamFaultEnSV))
            os << "Enabled"; else os << "Disabled";
        os << std::endl;

        uint8_t aux8[FW_NUM_BEAM_DESTINATIONS]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        uint32_t aux32[2]={0,0};
        firmware->getMitigation(aux32);
        //      firmware->getMitigation(aux8);
        firmware->extractMitigation(aux32, aux8);
        os << "Mitigation=[";
        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
        {
            os << (int) aux8[i];
            if (i == FW_NUM_BEAM_DESTINATIONS - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        firmware->getFirmwareMitigation(aux32);
        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
            aux8[i]=0;
        //      firmware->getFirmwareMitigation(aux8);
        firmware->extractMitigation(aux32, aux8);
        os << "FirmwareMitigation=[";
        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
        {
            os << (int) aux8[i];
            if (i == FW_NUM_BEAM_DESTINATIONS - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        firmware->getSoftwareMitigation(aux32);
        os << "SoftwareMitigation=[";
        os << std::hex << "0x" << aux32[0] << " 0x" << aux32[1] << std::dec << "]" << std::endl;

        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
            aux8[i]=0;
        //      firmware->getSoftwareMitigation(aux8);
        firmware->extractMitigation(aux32, aux8);
        os << "SoftwareMitigation=[";
        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
        {
            os << (int) aux8[i];
            if (i == FW_NUM_BEAM_DESTINATIONS - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        firmware->getLatchedMitigation(aux32);
        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
            aux8[i]=0;
        firmware->extractMitigation(aux32, aux8);
        //      firmware->getLatchedMitigation(aux8);
        os << "LatchedMitigation=[";
        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
        {
            os << (int) aux8[i];
            if (i == FW_NUM_BEAM_DESTINATIONS - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
            aux[i]=0;
        firmware->_monitorRxErrorCntSV->getVal(aux, FW_NUM_BEAM_DESTINATIONS);
        os << "MonitorRxErrorCnt=[";
        for (uint32_t i = 0; i < FW_NUM_CONNECTIONS; ++i)
        {
            os << (int) aux[i];
            if (i == FW_NUM_CONNECTIONS - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
            aux[i]=0;
        firmware->_monitorPauseCntSV->getVal(aux, FW_NUM_BEAM_DESTINATIONS);
        os << "MonitorPauseCnt=[";
        for (uint32_t i = 0; i < FW_NUM_CONNECTIONS; ++i)
        {
            os << (int) aux[i];
            if (i == FW_NUM_CONNECTIONS - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
            aux[i]=0;
        firmware->_monitorOvflCntSV->getVal(aux, FW_NUM_BEAM_DESTINATIONS);
        os << "MonitorOvflCnt=[";
        for (uint32_t i = 0; i < FW_NUM_CONNECTIONS; ++i)
        {
            os << (int) aux[i];
            if (i == FW_NUM_CONNECTIONS - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        for (uint32_t i = 0; i < FW_NUM_BEAM_DESTINATIONS; ++i)
            aux[i]=0;
        firmware->_monitorDropCntSV->getVal(aux, FW_NUM_BEAM_DESTINATIONS);
        os << "MonitorDropCnt=[";
        for (uint32_t i = 0; i < FW_NUM_CONNECTIONS; ++i)
        {
            os << (int) aux[i];
            if (i == FW_NUM_CONNECTIONS - 1)
                os << "]"; else os << ", ";
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
        if (firmware->getBoolU64(firmware->_timeoutEnableSV))
            os << "Enabled"; else os << "Disabled";
        os << std::endl;

        os << "TimeoutTime="
            << firmware->getUInt32(firmware->_timeoutTimeSV) << " usec"  << std::endl;
        firmware->_timeoutTimeSV->setVal(7);

        os << "TimeoutMsgVer="
            << firmware->getUInt32(firmware->_timeoutMsgVerSV) << std::endl;

        os << "TimoutErrIndex=";
        uint32_t aux32_32[32];
        for (uint32_t i = 0; i < 32; ++i)
            aux32_32[i]=0;
        firmware->_timeoutErrIndexSV->getVal(aux32_32, 32);
        for (uint32_t i = 0; i < 32; ++i)
        {
            os << "0x" << std::hex << aux32_32[i] << std::dec;
            if (i == 32 - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;


        os << "TimoutMask=";
        for (uint32_t i = 0; i < 32; ++i)
            aux32_32[i]=0;
        firmware->_timeoutMaskSV->getVal(aux32_32, 32);
        for (uint32_t i = 0; i < 32; ++i)
        {
            os << "0x" << std::hex << aux32_32[i] << std::dec;
            if (i == 32 - 1)
                os << "]"; else os << ", ";
        }
        os << std::endl;

        os << "EvaluationEnable=";
        if (firmware->getBoolU64(firmware->_evaluationEnableSV))
            os << "Enabled"; else os << "Disabled";
        os << std::endl;

        os << "SoftwareWdTime="
            << firmware->getUInt32(firmware->_swWdTimeSV) << std::endl;

        os << "EvaluationTimeStamp="
            << firmware->getUInt32(firmware->_evaluationTimeStampSV) << std::endl;

    }
    catch (IOError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
    }
    catch (InvalidArgError &e)
    {
        std::cout << "Exception Info: " << e.getInfo() << std::endl;
    }

    return os;
}

#endif
