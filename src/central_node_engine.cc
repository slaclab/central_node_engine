#include <central_node_engine.h>
#include <central_node_firmware.h>
#include <central_node_history.h>

#include <stdio.h>
#include <stdint.h>
#include <sched.h>
#include <sys/mman.h>

#include <log.h>
#include <log_wrapper.h>

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
using namespace easyloggingpp;
static Logger *engineLogger;
#endif

std::mutex      Engine::_mutex;
volatile bool   Engine::_evaluate;
uint32_t        Engine::_rate;
uint32_t        Engine::_updateCounter;
time_t          Engine::_startTime;
uint32_t        Engine::_inputUpdateFailCounter;
bool            Engine::_enableMps = false;

Engine::Engine()
:
    _initialized(false),
    _engineThread(NULL),
    _debugCounter(0),
    _aomAllowWhileShutterClosed(false),
    _aomAllowEnableCounter(0),
    _aomAllowDisableCounter(0),
    _linacFwLatch(false),
    _reloadCount(0),
    _checkFaultTime( "Evaluation only time", 720 ),
    _evaluationCycleTime( "Evaluation Cycle time", 720 ),
    hb( Firmware::getInstance().getRoot(), 3500, 720 )
{
#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
    engineLogger = Loggers::getLogger("ENGINE");
    LOG_TRACE("ENGINE", "Created Engine");
#endif

    Engine::_evaluate = false;
    Engine::_rate = 0;
    Engine::_updateCounter = 0;
    Engine::_startTime = 0;
    Engine::_inputUpdateFailCounter = 0;

    // Lock memory
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1)
    {
        perror("mlockall failed");
        std::cerr << "WARN: Engine::Engine() mlockall failed." << std::endl;
    }
}

Engine::~Engine()
{
#if defined(LOG_ENABLED)
    //  Firmware::getInstance().showStats();
#endif
    // Stop the MPS before quitting
    Firmware::getInstance().setSoftwareEnable(false);
    Firmware::getInstance().setEnable(false);

    _checkFaultTime.show();

    _evaluate = false;
    threadJoin();
    std::cout << "INFO: Stopping bypass thread now..." << std::endl;
    _bypassManager->stopBypassThread();
}

MpsDbPtr Engine::getCurrentDb()
{
    return _mpsDb;
}

BypassManagerPtr Engine::getBypassManager()
{
    return _bypassManager;
}

/**
 * This is called by the bypass thread when one or multiple
 * bypasses expired.
 */
int Engine::reloadConfig()
{
    // If updateThread active, then stop it first
    if (_evaluate)
    {
        _evaluate = false;
        threadJoin();
    }

    {
        std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());
        _mpsDb->writeFirmwareConfiguration();
    }

    _evaluate = true;

    startUpdateThread();

    return 0;
}

// This is called when analog devices need to be ignored (and un-ignored)
int Engine::reloadConfigFromIgnore()
{
    Firmware::getInstance().setEvaluationEnable(false);
    Firmware::getInstance().setSoftwareEnable(false);
    Firmware::getInstance().setEnable(false);

    {
        std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());
        _mpsDb->writeFirmwareConfiguration(false, _aomAllowWhileShutterClosed);
    }

    Firmware::getInstance().setEnable(true);
    Firmware::getInstance().clearAll();
    //  Firmware::getInstance().softwareClear();
    Firmware::getInstance().setSoftwareEnable(true);
    Firmware::getInstance().setEvaluationEnable(true);

    ++_reloadCount;

    return 0;
}

int Engine::loadConfig(std::string yamlFileName, uint32_t inputUpdateTimeout)
{
    if (_mpsDb)
    {
        std::cout << "INFO: Database already loaded, must reboot IOC to load again" << std::endl;
        return 1;
    }

    std::cout << "INFO: Engine::loadConfig(" << yamlFileName << ")" << std::endl;
    std::unique_lock<std::mutex> engineLock(_mutex);

    // First stop the MPS
    Firmware::getInstance().setSoftwareEnable(false);
    Firmware::getInstance().setEnable(false);

    // If updateThread active, then stop it first - the _mpsDb check will prevents
    // a database from being reloaded, no need to stop the updateThread
    if (_evaluate)
    {
        _evaluate = false;
        engineLock.unlock();
        threadJoin();
        engineLock.lock();
    }

    _evaluate = false;

    MpsDb *db = new MpsDb(inputUpdateTimeout);
    boost::shared_ptr<MpsDb> mpsDb = boost::shared_ptr<MpsDb>(db);

    {
        std::unique_lock<std::mutex> lock(*mpsDb->getMutex()); // the database mutex is static - is the same for all instances

        if (mpsDb->load(yamlFileName) != 0)
        {
            _errorStream.str(std::string());
            _errorStream << "ERROR: Failed to load yaml database ("
                << yamlFileName << ")";
            throw(EngineException(_errorStream.str()));
        }

        LOG_TRACE("ENGINE", "YAML Database loaded from " << yamlFileName);

        // When the first yaml configuration file is loaded the bypassMap
        // must be created.
        //
        // TODO/Database/Important: the bypasses are created for each device input and
        // analog channel in the database, this happens for the first time
        // a database is loaded. If there are subsequent database changes
        // they must have the same device inputs and analog channels.
        // Currently if the number of channels differ between the loaded
        // databases the central node engine must be restarted.
        if (!_bypassManager)
        {
            _bypassManager = BypassManagerPtr(new BypassManager());
            _bypassManager->createBypassMap(mpsDb);
            _bypassManager->startBypassThread();
        }

        LOG_TRACE("ENGINE", "BypassManager created");

        try
        {
            mpsDb->configure();
        }
        catch (DbException &e)
        {
            throw e;
        }

        LOG_TRACE("ENGINE", "MPS Database configured from YAML");

        // Assign bypass to each digital/analog input
        try
        {
            _bypassManager->assignBypass(mpsDb);
        }
        catch (CentralNodeException &e)
        {
            _initialized = true;
            _evaluate = true;
            startUpdateThread();
            throw e;
        }

        // Now that the database has been loaded and configured
        // successfully, assign to _mpsDb shared_ptr
        _mpsDb = mpsDb;//shared_ptr<MpsDb>(db);

        // Search for Mech. Shutter device - this is needed to keep track
        // of the shutter status, and allow AOM when the shutter is closed
        // If it is not found, we will let the system run, but we won't be
        // able to keep track of the shutter status. This is needed for the
        // dual-core operation.
        if (!findShutterDevice())
            std::cout << "Mech. Shutter device not found." << std::endl;


        // We allow the system to run without finding both the Linac and the AOM.
        // This is needed for the dual-core operation.
        if (!findBeamDestinations())
            std::cout << "AOM and/or Linac beam destination not found." << std::endl;

        // Find the lowest/highest BeamClasses - used when checking faults
        uint32_t num = 0;
        uint32_t lowNum = 100;
        for (DbBeamClassMap::iterator beamClass = _mpsDb->beamClasses->begin();
            beamClass != _mpsDb->beamClasses->end();
            ++beamClass)
        {
            if ((*beamClass).second->number > num)
            {
                _highestBeamClass = (*beamClass).second;
                num = (*beamClass).second->number;
            }

            if ((*beamClass).second->number < lowNum)
            {
                _lowestBeamClass = (*beamClass).second;
                lowNum = (*beamClass).second->number;
            }
        }

        _mpsDb->writeFirmwareConfiguration(true);
    }

    LOG_TRACE("ENGINE", "Lowest beam class found: " << _lowestBeamClass->number);
    LOG_TRACE("ENGINE", "Highest beam class found: " << _highestBeamClass->number);

    _initialized = true;

    _evaluate = true;
    engineLock.unlock();

    startUpdateThread();

    return 0;
}

bool Engine::findBeamDestinations()
{
    for (DbBeamDestinationMap::iterator it = _mpsDb->beamDestinations->begin();
        it != _mpsDb->beamDestinations->end();
        ++it)
    {
        if ((*it).second->name == "AOM")
            _aomDestination = (*it).second;
        else if ((*it).second->name == "LINAC")
            _linacDestination = (*it).second;
    }

    if (!_aomDestination or !_linacDestination)
        return false;

    return true;
}

bool Engine::findShutterDevice()
{
    for (DbDigitalDeviceMap::iterator device = _mpsDb->digitalDevices->begin();
        device != _mpsDb->digitalDevices->end();
        ++device)
    {
        if ((*device).second->name == "Mech. Shutter")
        {
            _shutterDevice = (*device).second;
            for (DbDeviceStateMap::iterator state = _shutterDevice->deviceType->deviceStates->begin();
                state != _shutterDevice->deviceType->deviceStates->end();
                ++state)
            {
                if ((*state).second->name == "CLOSED")
                {
                    _shutterClosedStatus = (*state).second->value;
                    return true;
                }
            }
        }
    }

    return false;
}

bool Engine::isInitialized()
{
    return _initialized;
}

/**
 * Prior to check digital/analog faults assign the highest beam class available
 * as tentativeBeamClass for all beam destinations. Also temporarily sets the
 * allowedBeamClass as the lowest beam class available.
 *
 * As faults are detected the tentativeBeamClass gets lowered accordingly.
 */
void Engine::setTentativeBeamClass()
{
    // Assigns _highestBeamClass as tentativeBeamClass for all BeamDestinations
    for (DbBeamDestinationMap::iterator device = _mpsDb->beamDestinations->begin();
        device != _mpsDb->beamDestinations->end(); ++device)
    {
        (*device).second->tentativeBeamClass = _highestBeamClass;
        (*device).second->previousAllowedBeamClass = (*device).second->allowedBeamClass; // for history purposes
        (*device).second->allowedBeamClass = _lowestBeamClass;
        LOG_TRACE("ENGINE", (*device).second->name << " tentative class set to: "
            << (*device).second->tentativeBeamClass->number
            << "; allowed class set to: "
            << (*device).second->allowedBeamClass->number);
    }
}

/**
 * After checking the faults move the final tentativeBeamClass into the
 * allowedBeamClass for the beam destinations
 */
bool Engine::setAllowedBeamClass()
{
    // Set the allowed classes according to the SW evaluation
    for (DbBeamDestinationMap::iterator it = _mpsDb->beamDestinations->begin();
        it != _mpsDb->beamDestinations->end();
        ++it)
    {
        (*it).second->setAllowedBeamClass();
        //    (*it).second->allowedBeamClass = (*it).second->tentativeBeamClass;
        LOG_TRACE("ENGINE", (*it).second->name << " allowed class set to "
            << (*it).second->allowedBeamClass->number);
    }

    return true;
}

/**
 * We need to enable the AOM if the shutter is closed - this requires
 * a reload of the FW configuration at the moment the shutter is closed
 * and it was supposed to be close.
 *
 * Enabling the AOM means to remove the AOM destination from the FW
 * configuration and also set it to the highest class in SW).
 *
 * When the SW/FW evaluation conditions allow beam (power class
 * higher than PC0) then the shutter and AOM are allowed.
 *
 * 1) Enable AOM when shutter closes:
 * - Shutter is CLOSED; and
 * - Shutter is supposed to be closed: Linac mitigation read from FW is 0 (PC0)
 *   [Observation: the PC0 is caused by SW or FW logic evaluation, OR
 *    CN getting timeouts from apps, Watchdog faults, etc]
 *
 * -> This condition causes the SW to force Linac to PC0
 * -> The FW configuration is reloaded with AOM destination out
 *    The _forceAom=true causes the FW configuration to bypass AOM
 *    In SW the AOM destination receives the _highestBeamClass to allow beam
 * -> If the FW mitigation is latched (read from FW), then make the
 *    also SW mitigation latch.
 *    * The SW latch needs to be cleared by operators (clearSoftwareLatch() method)
 * -> Set _aomAllowWhileShutterClosed to true if it is not set
 * -> Do not reload if this condition was active on the previous cycle
 *    Condition signaled by _aomAllowWhileShutterClosed variable
 *
 * 2) Condition to restore AOM mitigation by SW/FW
 * - Shutter is NOT CLOSED; and
 * - _aomAllowWhileShutterClosed is active (i.e. AOM mitigation disabled,
 *   allowing beam through AOM); and
 * - Shutter is supposed to be opened:
 *   * Linac mitigation read from FW is not 0; and
 *   * Linac FW latch mitigation read from FW is not 0
 * -> Reset _aomAllowWhileShutterClosed (set false)
 * -> This causes the FW configuration to be reloaded with AOM destination in
 *
 * Return true if FW configuration must be reloaded.
 *
 * Note: this check will run ony when:
 * - Both the Linac and the AOM destination, and
 * - The Mech. shutter device
 * are all defined in the DB. Otherwise, this check will
 * do nothing and simply return false.
 */
bool Engine::checkAomState()
{
    // Check if the shutter device and both the Linac and AOM destinations
    // are defined. Otherwise just reutrn false.
    if (!_aomDestination or !_linacDestination or !_shutterDevice)
        return false;

    bool reload = false;

    // Check whether there is an active FW latch mitigation
    uint32_t latchedMitigation[2];
    Firmware::getInstance().getLatchedMitigation(&latchedMitigation[0]);
    uint32_t linacLatchedMitigation = latchedMitigation[1] & 0xF;

    // Read the current mitigation (result of both SW/FW evaluations)
    uint32_t mitigation[2];
    Firmware::getInstance().getMitigation(&mitigation[0]);
    uint32_t linacMitigation = mitigation[1] & 0xF;

    bool shutterClosed = false;
    if (_shutterDevice->value == _shutterClosedStatus)
        shutterClosed = true;

    // First condition - shutter is closed, it is supposed to be closed and
    // we are not in the force AOM mode already
    // then removes AOM from FW evaluation (by reloading)
    if (shutterClosed &&
        !_aomAllowWhileShutterClosed &&
        shouldShutterBeClosed(linacMitigation==0))
    {
        // If the FW linac mitigation is ON then latch it in SW, because the FW
        // reload operation will unlatch it
        if (linacLatchedMitigation == 0)
            _linacFwLatch = true; // This is cleared only by OPS

        _aomAllowWhileShutterClosed = true; // This causes AOM to be bypassed by FW
        ++_aomAllowEnableCounter;
        reload = true;
    }

    // Set PC0 to shutter in SW if it was latched in FW before reload
    if (_linacFwLatch)
        _linacDestination->tentativeBeamClass = _lowestBeamClass;

    // Set high PC to AOM if _aomAllowWhileShutterClosed is active
    if (_aomAllowWhileShutterClosed)
        _aomDestination->tentativeBeamClass = _highestBeamClass;


    // Second condition - shutter is closed, but there are no faults,
    // it could be opened, first close AOM
    if (shutterClosed && _aomAllowWhileShutterClosed &&
        shouldShutterBeOpened(linacLatchedMitigation==0, linacMitigation==0))
    {
        _aomDestination->tentativeBeamClass = _lowestBeamClass;
        _aomAllowWhileShutterClosed = false;
        ++_aomAllowDisableCounter;
        reload = true;
    }
    // Otherwise, if shutter is not closed - then disallow AOM
    else if (!shutterClosed && _aomAllowWhileShutterClosed)
    {
        ++_aomAllowDisableCounter;
        _aomAllowWhileShutterClosed = false;
        reload = true;
    }

    return reload;
}

/**
 * Returns whether the shutter is supposed to be closed.
 */
bool Engine::shouldShutterBeClosed(bool linacMitigation)
{
    bool shouldBeClosed = linacMitigation || _linacFwLatch;
    return shouldBeClosed; // TODO: Include other conditions, e.g. app timed out
}

/**
 * Retuns whether the shutter is supposed to be opened
 */
bool Engine::shouldShutterBeOpened(bool linacFwLatchedMitigation, bool linacMitigation)
{
    bool swFaultActive = true;
    if (_linacDestination->tentativeBeamClass != _lowestBeamClass)
        swFaultActive = false;

    bool shouldBeOpened = !swFaultActive && !linacFwLatchedMitigation && !linacMitigation;

    return shouldBeOpened;
}

/**
 * First goes through the list of DigitalDevices and updates the current state.
 * Second updates the FaultStates for each Fault.
 * At the end of this method all Digital Faults will have updated values,
 * i.e. the field 'faulted' gets updated based on the current machine state.
 *
 * TODO: bypass mask and values accessed by multiple threads
 */
void Engine::evaluateFaults()
{
    uint32_t deviceStateId = 0;

    bool faulted = false;

    // Update digital device values based on individual inputs
    // The individual inputs come from independent digital inputs,
    // this does not apply to analog devices, where the input
    // come from a single channel (with multiple bits in it)
    for (DbDigitalDeviceMap::iterator device = _mpsDb->digitalDevices->begin();
        device != _mpsDb->digitalDevices->end(); ++device)
    {
        // If device has no card assigned, then it cannot be evaluated.
        if ((*device).second->cardId != NO_CARD_ID &&
            (*device).second->evaluation != NO_EVALUATION)
        {
            uint32_t deviceValue = 0;
            LOG_TRACE("ENGINE", "Getting inputs for " << (*device).second->name << " device"
                << ", there are " << (*device).second->inputDevices->size()
                << " inputs");

            for (DbDeviceInputMap::iterator input = (*device).second->inputDevices->begin();
                input != (*device).second->inputDevices->end();
                ++input)
            {
                uint32_t inputValue = 0;
                // Check if the input has a valid bypass value
                if ((*input).second->bypass->status == BYPASS_VALID)
                {
                    inputValue = (*input).second->bypass->value;
                    LOG_TRACE("ENGINE", (*device).second->name << " bypassing input value to "
                        << (*input).second->bypass->value << " (actual value is "
                        << (*input).second->latchedValue << ")");
                }
                else
                {
                    inputValue = (*input).second->latchedValue;
                }

                inputValue <<= (*input).second->bitPosition;
                deviceValue |= inputValue;
            }

            (*device).second->update(deviceValue);

            LOG_TRACE("ENGINE", (*device).second->name << " current value " << std::hex << deviceValue << std::dec);
        }
    }

    // At this point all digital devices have an updated value

    // Update digital & analog Fault values and BeamDestination allowed class
    for (DbFaultMap::iterator fault = _mpsDb->faults->begin();
        fault != _mpsDb->faults->end();
        ++fault)
    {
        LOG_TRACE("ENGINE", (*fault).second->name << " updating fault values");
        uint32_t oldFaultValue = (*fault).second->value;
        (*fault).second->sendUpdate = 0;
        // First calculate the digital Fault value from its one or more digital device inputs
        uint32_t faultValue = 0;
        for (DbFaultInputMap::iterator input = (*fault).second->faultInputs->begin();
            input != (*fault).second->faultInputs->end();
            ++input)
        {
            uint32_t inputValue = 0;
            if ((*input).second->digitalDevice)
            {
                inputValue = (*input).second->digitalDevice->value;
            }
            else
            {
                // Check if there is an active bypass for analog device
                LOG_TRACE("ENGINE", (*input).second->analogDevice->name << " bypassMask=" << std::hex <<
                    (*input).second->analogDevice->bypassMask << ", value=" <<
                    (*input).second->analogDevice->value << std::dec);

                inputValue = (*input).second->analogDevice->value & (*input).second->analogDevice->bypassMask;
            }

            faultValue |= (inputValue << (*input).second->bitPosition);
            LOG_TRACE("ENGINE", (*fault).second->name << " current value " << std::hex << faultValue
                << ", input value " << inputValue << std::dec << " bit pos "
                << (*input).second->bitPosition);
        }

        (*fault).second->update(faultValue);
        (*fault).second->faulted = false; // Clear the fault - in case it was faulted before
        LOG_TRACE("ENGINE", (*fault).second->name << " current value " << std::hex << faultValue << std::dec);

        // Now that a Fault has a new value check if it is in the FaultStates list,
        // and update the allowedBeamClass for the BeamDestinations
        for (DbFaultStateMap::iterator state = (*fault).second->faultStates->begin();
            state != (*fault).second->faultStates->end();
            ++state)
        {
            (*state).second->ignored = false; // Mark not ignored - the ignore logic is evaluated later
            uint32_t maskedValue = faultValue & (*state).second->deviceState->mask;
            LOG_TRACE("ENGINE", (*fault).second->name << ", checking fault state ["
                << (*state).second->id << "]: "
                << "masked value is " << std::hex << maskedValue << " (mask="
                << (*state).second->deviceState->mask << ")" << std::dec);

            if ((*state).second->deviceState->value == maskedValue)
            {
                deviceStateId = (*state).second->deviceState->id;
                (*state).second->faulted = true; // Set input faulted field
                (*fault).second->faulted = true; // Set fault faulted field
                (*fault).second->faultLatched = true;
                faulted = true; // Signal that at least one state is faulted
                LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value="
                    << faultValue << ", masked=" << maskedValue
                    << " (fault state="
                    << (*state).second->deviceState->name
                    << ", value=" << (*state).second->deviceState->value << ")");
            }
            else
            {
                (*state).second->faulted = false;
            }
        }

        // If there are no faults, then enable the default - if there is one
        if (!faulted && (*fault).second->defaultFaultState)
        {
            (*fault).second->defaultFaultState->faulted = true;
            deviceStateId = (*fault).second->defaultFaultState->deviceState->id;
            LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value="
                << faultValue << " (Default) fault state="
                << (*fault).second->defaultFaultState->deviceState->name);
        }
        if (oldFaultValue != (*fault).second->value)
        {
            History::getInstance().logFault((*fault).second->id, oldFaultValue,
                (*fault).second->faulted, deviceStateId);
            (*fault).second->sendUpdate = 1;
        }

        deviceStateId = 0;
    }
}

// Check if there are valid ignore conditions. If changes in ignore condition
// requires firmware reconfiguration, then return true.
bool Engine::evaluateIgnoreConditions()
{
    bool reload = false;

    // Calculate state of conditions
    for (DbConditionMap::iterator condition = _mpsDb->conditions->begin();
        condition != _mpsDb->conditions->end();
        ++condition)
    {
        uint32_t conditionValue = 0;
        for (DbConditionInputMap::iterator input = (*condition).second->conditionInputs->begin();
            input != (*condition).second->conditionInputs->end(); ++input)
        {
            uint32_t inputValue = 0;
            if ((*input).second->faultState)
            {
                if ((*input).second->faultState->faulted)
                    inputValue = 1;
            }

            conditionValue |= (inputValue << (*input).second->bitPosition);
            LOG_TRACE("ENGINE", "Condition " << (*condition).second->name << " current value " << std::hex << conditionValue
                << ", input value " << inputValue << std::dec << " bit pos "
                << (*input).second->bitPosition);
        }


        // 'mask' is the condition value that needs to be matched in order to ignore faults
        bool newConditionState = false;
        if ((*condition).second->mask == conditionValue)
            newConditionState = true;

        (*condition).second->state = newConditionState;
        LOG_TRACE("ENGINE",  "Condition " << (*condition).second->name << " is " << (*condition).second->state);

        if ((*condition).second->ignoreConditions)
        {
            for (DbIgnoreConditionMap::iterator ignoreCondition = (*condition).second->ignoreConditions->begin();
                ignoreCondition != (*condition).second->ignoreConditions->end();
                ++ignoreCondition)
            {
                if ((*ignoreCondition).second->faultState)
                {
                    LOG_TRACE("ENGINE",  "Ignoring fault state [" << (*ignoreCondition).second->faultStateId << "]"
                        << ", state=" << (*condition).second->state);

                    (*ignoreCondition).second->faultState->ignored = (*condition).second->state;

                    // This check is needed in case specific faults from an AnalogDevice is
                    // listed in the ignoreCondition.
                    if ((*ignoreCondition).second->analogDevice)
                    {
                        int integrator = (*ignoreCondition).second->faultState->deviceState->getIntegrator();
                        if ((*ignoreCondition).second->analogDevice->ignoredIntegrator[integrator] != (*condition).second->state)
                            reload = true; // reload configuration!

                        (*ignoreCondition).second->analogDevice->ignoredIntegrator[integrator] = (*condition).second->state;
                    }
                }
                else
                {
                    if ((*ignoreCondition).second->analogDevice)
                    {
                        LOG_TRACE("ENGINE",  "Ignoring analog device [" << (*ignoreCondition).second->analogDeviceId << "]"
                            << ", state=" << (*condition).second->state);

                        if ((*ignoreCondition).second->analogDevice->ignored != (*condition).second->state)
                            reload = true; // reload configuration!

                        (*ignoreCondition).second->analogDevice->ignored = (*condition).second->state;
                    }
                }
            }
        }
    }

    return reload;
}

void Engine::mitigate()
{
    // Update digital Fault values and BeamDestination allowed class
    for (DbFaultMap::iterator fault = _mpsDb->faults->begin();
        fault != _mpsDb->faults->end();
        ++fault)
    {
        uint32_t maximumClass = 100;
        uint32_t sendFaultId = (*fault).second->id;
        uint32_t sendOldValue = (*fault).second->oldValue;
        uint32_t sendValue = (*fault).second->value;
        uint32_t sendAllowClass = 0;
        // Mitigate only those faults that are the result of slow evaluation
#ifdef FAST_SW_EVALUATION
#warning "Code compiled to evaluate fast rules - FOR TESTING ONLY!"
        if (true)
        {
#else
        if ((*fault).second->evaluation == SLOW_EVALUATION) {
#endif
            for (DbFaultStateMap::iterator state = (*fault).second->faultStates->begin();
                state != (*fault).second->faultStates->end();
                ++state)
            {
                if ((*state).second->faulted && !(*state).second->ignored)
                {
                    LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value="
                        << (*fault).second->value
                        << " (fault state="
                        << (*state).second->deviceState->name
                        << ", value=" << (*state).second->deviceState->value << ")");

                    if ((*state).second->allowedClasses)
                    {
                        for (DbAllowedClassMap::iterator allowed = (*state).second->allowedClasses->begin();
                            allowed != (*state).second->allowedClasses->end();
                            ++allowed)
                        {
                            if ((*allowed).second->beamDestination->tentativeBeamClass->number >=
                                (*allowed).second->beamClass->number)
                            {
                                (*allowed).second->beamDestination->tentativeBeamClass =
                                    (*allowed).second->beamClass;

                                LOG_TRACE("ENGINE", (*allowed).second->beamDestination->name
                                    << " tentative class set to "
                                    << (*allowed).second->beamClass->number);
                                if ((*fault).second->sendUpdate) {
                                  if ((*allowed).second->beamClass->number < maximumClass) {
                                    maximumClass = (*allowed).second->beamClass->number;
                                    sendAllowClass = (*allowed).second->id;
                                  }
                                }
                            }
                        }
                    }
                    else
                    {
                        LOG_TRACE("ENGINE", "WARN: no AllowedClasses found for "
                            << (*fault).second->name << " fault");
                    }
                }
            }
            // If there are no faults, then enable the default - if there is one
            if ((*fault).second->defaultFaultState)
            {
                if ((*fault).second->defaultFaultState->faulted &&
                    !(*fault).second->defaultFaultState->ignored)
                {
                    LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value="
                        << (*fault).second->value << " (Default) fault state="
                        << (*fault).second->defaultFaultState->deviceState->name);

                    if ((*fault).second->defaultFaultState->allowedClasses)
                    {
                        for (DbAllowedClassMap::iterator allowed = (*fault).second->defaultFaultState->allowedClasses->begin();
                            allowed != (*fault).second->defaultFaultState->allowedClasses->end();
                            ++allowed)
                        {
                            if ((*allowed).second->beamDestination->tentativeBeamClass->number >
                                (*allowed).second->beamClass->number)
                            {
                                (*allowed).second->beamDestination->tentativeBeamClass =
                                    (*allowed).second->beamClass;

                                LOG_TRACE("ENGINE", (*allowed).second->beamDestination->name << " tentative class set to "
                                    << (*allowed).second->beamClass->number);
                            }
                        }
                    }
                }
            }
        }
        // Fast fault - just log the mitigation to history server, but don't mitigate
        else {
            for (DbFaultStateMap::iterator state = (*fault).second->faultStates->begin();
                state != (*fault).second->faultStates->end();
                ++state)
            {
                if ((*state).second->faulted && !(*state).second->ignored)
                {
                    if ((*state).second->allowedClasses)
                    {
                        for (DbAllowedClassMap::iterator allowed = (*state).second->allowedClasses->begin();
                            allowed != (*state).second->allowedClasses->end();
                            ++allowed)
                        {
                            if ((*allowed).second->beamDestination->tentativeBeamClass->number >=
                                (*allowed).second->beamClass->number)
                            {
                                if ((*fault).second->sendUpdate) {
                                  if ((*allowed).second->beamClass->number < maximumClass) {
                                    maximumClass = (*allowed).second->beamClass->number;
                                    sendAllowClass = (*allowed).second->id;
                                  }
                                }
                            }
                        }
                    }
                    else
                    {
                        LOG_TRACE("ENGINE", "WARN: no AllowedClasses found for "
                            << (*fault).second->name << " fault");
                    }
                }
            }
        }
        if (maximumClass < 100) {
          History::getInstance().logMitigation(sendFaultId,sendOldValue,sendValue,sendAllowClass);
        }
        else {
          if ((*fault).second->sendUpdate) {
            History::getInstance().logMitigation(sendFaultId,sendOldValue,sendValue,sendAllowClass);
          }
        }
    }
}

int Engine::checkFaults()
{
    if (!_mpsDb)
        return 0;

    // must first get updated input values
    _checkFaultTime.start();

    LOG_TRACE("ENGINE", "Checking faults");
    bool reload = false;
    {
        std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());
        _mpsDb->clearMitigationBuffer();
        setTentativeBeamClass();
        evaluateFaults();
        reload = evaluateIgnoreConditions();
        mitigate();
        if (checkAomState())
            reload = true;

        setAllowedBeamClass();
    }

    _checkFaultTime.tick();

    // If FW configuration needs reloading, return non-zero value
    if (reload)
        return 1;

    return 0;
}

/**
 * This releases the software mitigation state latched because
 * the FW was latched - this is required in order to avoid loosing
 * latch after a FW configuration reload.
 */
void Engine::clearSoftwareLatch()
{
    _linacFwLatch = false;
}

void Engine::showFaults()
{
    if (!isInitialized())
        return;

    // Print current faults
    bool faults = false;

    // Digital Faults
    {
        std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());
        for (DbFaultMap::iterator fault = _mpsDb->faults->begin();
            fault != _mpsDb->faults->end();
            ++fault)
        {
            for (DbFaultStateMap::iterator state = (*fault).second->faultStates->begin();
                state != (*fault).second->faultStates->end();
                ++state)
            {
                if ((*state).second->faulted)
                {
                    if (!faults)
                    {
                        std::cout << "# Current faults:" << std::endl;
                        faults = true;
                    }

                    std::cout << "  " << (*fault).second->name << ": " << (*state).second->deviceState->name
                        << " (value=" << (*state).second->deviceState->value << ", ignored="
                        << (*state).second->ignored << ")" << std::endl;
                }
            }
        }
    }

    if (!faults)
        std::cout << "# No faults" << std::endl;
}

void Engine::showStats()
{
    if (isInitialized())
    {
        std::cout << ">>> Engine Stats <<<" << std::endl;
        _checkFaultTime.show();
        _evaluationCycleTime.show();
        hb.printReport();
        std::cout << "Rate: " << Engine::_rate << " Hz" << std::endl;

        if (_shutterDevice)
            std::cout << "Shutter Status: " << Engine::_shutterDevice->value
                << " (CLOSED=" << Engine::_shutterClosedStatus << ")" << std::endl;

        if (_aomDestination)
        {
            std::cout << "AOM Status: ";
            if (Engine::_aomAllowWhileShutterClosed)
                std::cout << " ALLOWED ";
            else
                std::cout << " Normal ";

            std::cout << "[" << Engine::_aomAllowEnableCounter << "/"
                << Engine::_aomAllowDisableCounter << "]" << std::endl;
        }

        std::cout << "Reload latch: " << Engine::_linacFwLatch << std::endl;
        std::cout << "Reload Config Count: " << Engine::_reloadCount << std::endl;

        if (_aomDestination)
            std::cout << "Allow AOM (shutter closed): " << Engine::_aomAllowWhileShutterClosed << std::endl;

        std::cout << "Counter: " << Engine::_updateCounter << std::endl;
        std::cout << "Input Update Fail Counter: " << Engine::_inputUpdateFailCounter
            << " (timed out waiting on FW 360Hz updates)" << std::endl;
        std::cout << "Started at " << ctime(&Engine::_startTime) << std::endl;
        std::cout << &History::getInstance() << std::endl;
        _debugCounter = 0;
    }
    else
    {
        std::cout << "MPS not initialized - no database" << std::endl;
    }
}

void Engine::showFirmware()
{
    std::cout << &(Firmware::getInstance());
}

void Engine::showBeamDestinations()
{
    if (!isInitialized())
    {
        std::cout << "MPS not initialized - no database" << std::endl;
        return;
    }

    std::cout << ">> Beam Destinations: " << std::endl;

    std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());

    for (DbBeamDestinationMap::iterator destination = _mpsDb->beamDestinations->begin(); destination != _mpsDb->beamDestinations->end(); ++destination)
    {
        std::cout << (*destination).second->name;
        if ((*destination).second->allowedBeamClass && (*destination).second->tentativeBeamClass)
        {
            std::cout << ":\t Allowed " << (*destination).second->allowedBeamClass->number
                << "/ Tentative " << (*destination).second->tentativeBeamClass->number
                << std::endl;
        }
        else
        {
            std::cout << ":\t ERROR -> no beam class assigned (is MPS disabled?)" << std::endl;
        }
    }

}

void Engine::showDeviceInputs()
{
    if (!isInitialized())
    {
        std::cout << "MPS not initialized - no database" << std::endl;
        return;
    }

    std::cout << "Device Inputs: " << std::endl;
    std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());

    for (DbDeviceInputMap::iterator input = _mpsDb->deviceInputs->begin(); input != _mpsDb->deviceInputs->end(); ++input)
    {
        std::cout << (*input).second << std::endl;
    }
}

void Engine::showDatabaseInfo()
{
    if (!isInitialized())
    {
        std::cout << "MPS not initialized - no database" << std::endl;
        return;
    }

    _mpsDb->showInfo();
}

void Engine::startUpdateThread()
{
    _engineThread = new std::thread(&Engine::engineThread, this);

    if (pthread_setname_np(_engineThread->native_handle(), "EngineThread"))
        perror("pthread_setname_np failed");
}

uint32_t Engine::getUpdateRate()
{
    return Engine::_rate;
}

uint32_t Engine::getUpdateCounter()
{
    return Engine::_updateCounter;
}

time_t Engine::getStartTime()
{
    return Engine::_startTime;
}

void Engine::threadExit()
{
    _evaluate = false;
}

void Engine::threadJoin()
{
    if (_engineThread != NULL)
    {
        std::cout << "INFO: Engine::threadJoin()" << std::endl;
        _engineThread->join();
    }
}

#define MAX_SAFE_STACK (8*1024) /* The maximum stack size which is
                                   guaranteed safe to access without
                                   faulting */

void stack_prefault(void)
{
    unsigned char dummy[MAX_SAFE_STACK];

    memset(dummy, 0, MAX_SAFE_STACK);
    return;
}

void Engine::engineThread()
{
    struct sched_param  param;

    std::cout << "INFO: Engine: update thread started." << std::endl;

    // Declare as real time task
    param.sched_priority = 88;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("Set priority");
        std::cerr << "WARN: Setting thread RT priority failed." << std::endl;
        //    exit(-1);
    }

    // Lock memory
    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1)
    {
        perror("mlockall failed");
        std::cerr << "WARN: mlockall failed." << std::endl;
    }

    // Pre-fault our stack
    stack_prefault();

    // Do not start MPS on the first time the thread runs (i.e.
    // when configuration is loaded)
    if (!_enableMps)
    {
        _enableMps = true;
    }
    else
    {
        Firmware::getInstance().setEnable(true);
        Firmware::getInstance().setSoftwareEnable(true);
    }

    Firmware::getInstance().clearAll();
    Firmware::getInstance().setTimingCheckEnable(true);

    time_t before = time(0);
    time_t now;
    _startTime = time(0);
    _updateCounter = 0;
    uint32_t counter = 0;
    bool reload = false;

    std::defer_lock_t t;
    std::unique_lock<std::mutex> engineLock(_mutex, t);

    while(_evaluate)
    {
        engineLock.lock();

        if (Engine::getInstance()._mpsDb)
        {
            // Wait for inputs to be updated
            {
                std::unique_lock<std::mutex> lock(*(Engine::getInstance()._mpsDb->getInputUpdateMutex()));
                while(!Engine::getInstance()._mpsDb->isInputReady())
                {
                    Engine::getInstance()._mpsDb->getInputUpdateCondVar()->wait_for(lock, std::chrono::milliseconds(5));
                    if (!_evaluate)
                    {
                        engineLock.unlock();
                        std::cout << "INFO: EngineThread: Exiting..." << std::endl;
                        return;
                    }
                }
            }

            reload = false;
            if (Engine::getInstance().checkFaults() > 0)
            {
                reload = true;
            }

            // Inputs were processed
            Engine::getInstance()._mpsDb->inputProcessed();

            // The mitigation buffer is ready, push it to the queue.
            Engine::getInstance()._mpsDb->pushMitBuffer();

            _updateCounter++;
            counter++;
            now = time(0);
            if (now > before)
            {
                time_t diff = now - before;
                before = now;
                _rate = counter / diff;
                counter = 0;
            }

            // Send a heartbeat
            Engine::getInstance().hb.beat();

            Engine::getInstance()._evaluationCycleTime.tick();

            // Reloads FW configuration - cause by ignore logic that
            // enables/disables faults based on fast analog devices
            if (reload)
            {
                Engine::getInstance().reloadConfigFromIgnore();
            }
        }
        else
        {
            _rate = 0;
            sleep(1);
        }

        engineLock.unlock();
    }

    Firmware::getInstance().setSoftwareEnable(false);
    Firmware::getInstance().setEnable(false);

    std::cout << "INFO: EngineThread: Exiting..." << std::endl;
}

long Engine::getMaxCheckTime()
{
    return static_cast<int>( _checkFaultTime.getAllMaxPeriod() * 1e6 );
}

long Engine::getAvgCheckTime()
{
    return static_cast<int>( _checkFaultTime.getMeanPeriod() * 1e6 );
}

long Engine::getMaxEvalTime()
{
    return static_cast<int>( _evaluationCycleTime.getAllMaxPeriod() * 1e6 );
}

long Engine::getAvgEvalTime()
{
    return static_cast<int>( _evaluationCycleTime.getMeanPeriod() * 1e6 );
}

void Engine::clearMaxTimers()
{
    hb.clear();
    _checkFaultTime.clear();
    _evaluationCycleTime.clear();
    _mpsDb->clearUpdateTime();
}

long Engine::getAvgWdUpdatePeriod()
{
    return static_cast<long>( hb.getMeanTxPeriod() * 1e6 );
}

long Engine::getMaxWdUpdatePeriod()
{
    return static_cast<long>( hb.getMaxTxPeriod() * 1e6 );
}
