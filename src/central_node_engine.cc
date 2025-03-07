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
    _unlatchAllowed(false),
    _linacFwLatch(false),
    _reloadCount(0),
    _checkFaultTime( "Evaluation only time: checkFaults()", 720 ),
    _evaluationCycleTime( "Evaluation Cycle time: 360 Hz time", 720 ),
    _unlatchTimer( "Unlatch timer",720 ),
    _setTentativeBeamClassTimer( "Set Tentative Beam Class", 720 ),
    _evaluateFaultsTimer( "evaluateFaultsTimer", 720 ),
    _setChannelIgnoreTimer( "setChannelIgnoreTimer", 720 ),
    _evaluateIgnoreConditionsTimer( "evaluateIgnoreConditionsTimer", 720 ),
    _mitigateTimer( "mitigateTimer", 720 ),
    _setAllowedBeamClassTimer( "setAllowedBeamClassTimer", 720 ),
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
    Firmware::getInstance().setEvaluationEnable(false);
    Firmware::getInstance().setSoftwareEnable(false);
    Firmware::getInstance().setEnable(false);

    {
        std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());
        _mpsDb->writeFirmwareConfiguration(false);
    }

    Firmware::getInstance().setEnable(true);
    Firmware::getInstance().setSoftwareEnable(true);
    Firmware::getInstance().setEvaluationEnable(true);
    Firmware::getInstance().clearAll();

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
        _mpsDb->writeFirmwareConfiguration(false);
    }

    Firmware::getInstance().setEnable(true);
    Firmware::getInstance().setSoftwareEnable(true);
    Firmware::getInstance().setEvaluationEnable(true);
    Firmware::getInstance().clearAll();

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
            _bypassManager->assignBypass();
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
    _setTentativeBeamClassTimer.start();
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
    _setTentativeBeamClassTimer.tick();
    _setTentativeBeamClassTimer.stop();
}

/**
 * After checking the faults move the final tentativeBeamClass into the
 * allowedBeamClass for the beam destinations
 */
bool Engine::setAllowedBeamClass()
{
    _setAllowedBeamClassTimer.start();
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
    _setAllowedBeamClassTimer.tick();
    _setAllowedBeamClassTimer.stop();
    return true;
}

/**
 * Set the ignored field for each analog/digital channel.
 * Based off modeActive (True - SC, False - NC)
 * Ignore logic is focused at the fault level, but channel ignore is needed for appCard ignores
 */
void Engine::setChannelIgnore() {
    _setChannelIgnoreTimer.start();
    for (DbDigitalChannelMap::iterator channel = _mpsDb->digitalChannels->begin();
        channel != _mpsDb->digitalChannels->end(); 
        ++channel)
    {
        // If channel has no card assigned, then it cannot be evaluated.
        if ((*channel).second->cardId != NO_CARD_ID &&
            (*channel).second->evaluation != NO_EVALUATION)
        {
            (*channel).second->ignored = !(*channel).second->modeActive;
        }
    }
    
    for (DbAnalogChannelMap::iterator channel = _mpsDb->analogChannels->begin();
        channel != _mpsDb->analogChannels->end(); ++channel)
    {
        // If channel has no card assigned, then it cannot be evaluated.
        if ((*channel).second->cardId != NO_CARD_ID &&
            (*channel).second->evaluation != NO_EVALUATION)
        {
          (*channel).second->ignored = !(*channel).second->modeActive;
        }
    }
    _setChannelIgnoreTimer.tick();
    _setChannelIgnoreTimer.stop();
}

/**
 * Updates the FaultStates for each Fault based off channel values.
 * At the end of this method all Faults will have updated values,
 * i.e. the field 'faulted' gets updated based on the current machine state.
 */
void Engine::evaluateFaults()
{
    _evaluateFaultsTimer.start();
    bool faulted = false;
    
    // Update digital & analog Fault values and BeamDestination allowed class
    for (DbFaultMap::iterator fault = _mpsDb->faults->begin();
        fault != _mpsDb->faults->end();
        ++fault)
    {
        LOG_TRACE("ENGINE", (*fault).second->name << " updating fault values");
        (*fault).second->sendUpdate = 0;
        uint32_t faultValue = 0;
        for (DbFaultInputMap::iterator input = (*fault).second->faultInputs->begin();
            input != (*fault).second->faultInputs->end();
            ++input)
        {
            int32_t inputValue = 0;
            if ((*input).second->digitalChannel)
            {
                // Calculate the digital Fault value from its one or more digital fault inputs
                if ((*input).second->digitalChannel->bypass->status == BYPASS_VALID)
                {
                    inputValue = (*input).second->digitalChannel->bypass->value;
                    LOG_TRACE("ENGINE", (*channel).second->name << " bypassing input value to "
                        << (*input).second->digitalChannel->bypass->value << " (actual value is "
                        << (*input).second->digitalChannel->latchedValue << ")");
                }
                else
                {
                    inputValue = (*input).second->digitalChannel->latchedValue; 
                }

                (*fault).second->faultedOffline = (*input).second->digitalChannel->faultedOffline;
                (*fault).second->faultActive = (*input).second->digitalChannel->modeActive;
            }
            else
            {
                // Calculate the analog fault value from one or more analog fault inputs
                LOG_TRACE("ENGINE", (*input).second->analogChannel->name << " bypassMask=" << std::hex <<
                    (*input).second->analogChannel->bypassMask << ", value=" <<
                    (*input).second->analogChannel->value << std::dec);

                inputValue = (*input).second->analogChannel->latchedValue & (*input).second->analogChannel->bypassMask;

                (*fault).second->faultedOffline = (*input).second->analogChannel->faultedOffline;
                (*fault).second->faultActive = (*input).second->analogChannel->modeActive;
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
            uint32_t maskedValue = faultValue & (*state).second->mask;
            LOG_TRACE("ENGINE", (*fault).second->name << ", checking fault state ["
                << (*state).second->id << "]: "
                << "masked value is " << std::hex << maskedValue << " (mask="
                << (*state).second->mask << ")" << std::dec);
            if ((*state).second->value == maskedValue)
            {
                (*state).second->active = true; // Set input active field
                faulted = true; // Signal that at least one state is faulted
                for (DbAllowedClassMap::iterator allowed = (*state).second->allowedClasses->begin();
                    allowed != (*state).second->allowedClasses->end();
                    ++allowed) {
                  if ((*allowed).second->beamClass->number < _highestBeamClass->number) {
                    (*fault).second->faulted = true; // Set fault faulted field
                    LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value="
                      << faultValue << ", masked=" << maskedValue
                      << " (fault state="
                      << (*state).second->name
                      << ", value=" << (*state).second->value << ")");
                  }
                }
            }
            else
            {
                (*state).second->active = false;
            }
        }

        // If there are no faults, then enable the default - if there is one
        if (!faulted && (*fault).second->defaultFaultState)
        {
            (*fault).second->defaultFaultState->active = true;
            LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value="
                << faultValue << " (Default) fault state="
                << (*fault).second->defaultFaultState->name);
        }
    }       
    _evaluateFaultsTimer.tick();
    _evaluateFaultsTimer.stop();
}

// Check if there are valid ignore conditions. If changes in ignore condition
// requires firmware reconfiguration, then return true.
bool Engine::evaluateIgnoreConditions()
{

    _evaluateIgnoreConditionsTimer.start();
    bool reload = false;
    std::set<uint32_t> checkAppCardIds; // Used to check if app card needs to be ignored if all its channels are ignored
    // Calculate state of conditions
    for (DbIgnoreConditionMap::iterator ignoreCondition = _mpsDb->ignoreConditions->begin();
        ignoreCondition != _mpsDb->ignoreConditions->end();
        ++ignoreCondition)
    {
        DbDigitalChannelPtr digitalChannel = (*ignoreCondition).second->digitalChannel;
        
        // The digitalChannel of the ignoreCondition should only have 1 faultInput
        uint32_t conditionValue = 0;
        if (digitalChannel->bypass->status == BYPASS_VALID)
            conditionValue = digitalChannel->bypass->value;
        else
            conditionValue = digitalChannel->latchedValue;
        // 'conditionValue' is the ignore condition value that needs to be matched in order to ignore fault
        bool newConditionState = false;
        if ((*ignoreCondition).second->value == conditionValue) {
            newConditionState = true;
        }
        
        if ((*ignoreCondition).second->state != newConditionState) {
          reload = true; // Set reload to true only if hasn't been reloaded already, hence why we store the state for ignoreCondition
        }
        (*ignoreCondition).second->state = newConditionState;
        LOG_TRACE("ENGINE",  "Ignore Condition " << (*ignoreCondition).second->name << " is " << (*ignoreCondition).second->state);

        // evaluate the faults associated with the ignore condition using "association_ignore" table
        for (DbFaultMap::iterator fault = (*ignoreCondition).second->faults->begin();
            fault != (*ignoreCondition).second->faults->end();
            fault++) 
        {
            if ((*fault).second->ignored != (*ignoreCondition).second->state) {
                (*fault).second->ignored = (*ignoreCondition).second->state;
                LOG_TRACE("ENGINE",  "Ignoring fault [" << (*fault).second->id << "]"
                    << ", state=" << (*ignoreCondition).second->state);
            }
        
            // Set fault fields to match its input fields
            for (DbFaultInputMap::iterator faultInput = (*fault).second->faultInputs->begin();
                faultInput != (*fault).second->faultInputs->end();
                ++faultInput)
            {
                if ((*faultInput).second->analogChannel) {
                    // Set analog channels ignore - used when writing analog configuration
                    if ((*faultInput).second->analogChannel->ignored != (*ignoreCondition).second->state) {
                        (*faultInput).second->analogChannel->ignored = (*ignoreCondition).second->state;
                        LOG_TRACE("ENGINE",  "Ignoring analog channel [" << (*faultInput).second->analogChannel->id << "]"
                            << ", state=" << (*ignoreCondition).second->state);
                        checkAppCardIds.insert((*faultInput).second->analogChannel->cardId);
                    }
                    
                }
                else {
                     // Set digital channels ignore - used for ignoring app cards
                    if ((*faultInput).second->digitalChannel->ignored != (*ignoreCondition).second->state) {
                        (*faultInput).second->digitalChannel->ignored = (*ignoreCondition).second->state;
                        LOG_TRACE("ENGINE",  "Ignoring digital channel [" << (*faultInput).second->digitalChannel->id << "]"
                            << ", state=" << (*ignoreCondition).second->state);
                        checkAppCardIds.insert((*faultInput).second->digitalChannel->cardId);
                        
                    }
                }
                
            }

        }
    }
    // Determine if application card needs to be ignored
    for (uint32_t appId : checkAppCardIds) {
        DbApplicationCardMap::iterator appCardIt = _mpsDb->applicationCards->find(appId);
        DbApplicationCardPtr appCard = (*appCardIt).second;
        bool ignoreAppCard = true;
        if (appCard->analogChannels) {
            for (DbAnalogChannelMap::iterator analogChannel = appCard->analogChannels->begin();
                analogChannel != appCard->analogChannels->end();
                analogChannel++) {
                if (!(*analogChannel).second->ignored) {
                    ignoreAppCard = false;
                    break;
                }
            }
        }
        else {
            for (DbDigitalChannelMap::iterator digitalChannel = appCard->digitalChannels->begin();
                digitalChannel != appCard->digitalChannels->end();
                digitalChannel++) {
                if (!(*digitalChannel).second->ignored) {
                    ignoreAppCard = false;
                    break;
                }
            }
        }
        appCard->ignoreStatus = ignoreAppCard;
    }

    _evaluateIgnoreConditionsTimer.tick();
    _evaluateIgnoreConditionsTimer.stop();
    return reload;
}

void Engine::mitigate()
{
    _mitigateTimer.start();
    // Update digital Fault values and BeamDestination allowed class
    for (DbFaultMap::iterator fault = _mpsDb->faults->begin();
        fault != _mpsDb->faults->end();
        ++fault)
    {
        uint32_t maximumClass = 100;
        uint32_t sendFaultId = (*fault).second->id;
        int32_t oldState = (*fault).second->displayState;
        int32_t currState = 0;
        if ((*fault).second->faulted) {
            currState = (*fault).second->displayState;
        }
        // If faultedOffline, then do not mitigate
        if ((*fault).second->faultedOffline) {
            currState = -1;
            // TODO: Add logic to not log offline faults, but ensure fault is logged
            // as it enters offline (add to history server)
        }
        else { 
            for (DbFaultStateMap::iterator state = (*fault).second->faultStates->begin();
                state != (*fault).second->faultStates->end();
                ++state)
            {
                if ((*state).second->active)
                {
                    LOG_TRACE("ENGINE", (*fault).second->name << " is faulted value="
                    << (*fault).second->value << " (fault state="
                    << (*state).second->name << ", value=" << (*state).second->value << ")");
                    for (DbAllowedClassMap::iterator allowed = (*state).second->allowedClasses->begin();
                        allowed != (*state).second->allowedClasses->end();
                        ++allowed)
                    {
                        if ((*allowed).second->beamDestination->tentativeBeamClass->number >=
                            (*allowed).second->beamClass->number)
                        {
                            if ((*fault).second->evaluation == SLOW_EVALUATION ) {
                                if ((*fault).second->ignored == false ) {
                                    (*allowed).second->beamDestination->tentativeBeamClass = (*allowed).second->beamClass;
                                }
                            }
                        }
                        if ((*allowed).second->beamClass->number < maximumClass) {
                            maximumClass = (*allowed).second->beamClass->number;
                            currState = (*state).second->id;                                        
                        }
                    }
                }
            }
        }
        if (currState != (*fault).second->displayState) {
            (*fault).second->sendUpdate = true;
            std::cout << (*fault).second->id << " sendUpdate - True\n"; // TEMP
        }
        else {
            (*fault).second->sendUpdate = false;
        }
        (*fault).second->displayState = currState;
        if ((*fault).second->sendUpdate && currState != -1) {
            History::getInstance().logFault(sendFaultId,oldState,currState);
        }
    }
    _mitigateTimer.tick();
    _mitigateTimer.stop();
}

int Engine::checkFaults()
{
    if (!_mpsDb)
        return 0;

    // must first get updated input values
    _checkFaultTime.start();
    LOG_TRACE("ENGINE", "Checking faults");
    bool reload = false;
    bool appReload = false;
    {
        std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());
        _mpsDb->clearMitigationBuffer();
        setTentativeBeamClass();
        setChannelIgnore();
        evaluateFaults();
        reload = evaluateIgnoreConditions();
        mitigate();
        setAllowedBeamClass();
        appReload = _mpsDb->getDbReload();
        _mpsDb->resetDbReload();
    }

    _checkFaultTime.tick();

    // If FW configuration needs reloading, return non-zero value
    if (reload || appReload) 
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
                if ((*state).second->active && (*state).second->name != "Is Ok")
                {
                    if (!faults)
                    {
                        std::cout << "# Current faults:" << std::endl;
                        faults = true;
                    }

                    std::cout << "  " << (*fault).second->name << ": " << (*state).second->name
                        << " (value=" << (*state).second->value << ", ignored="
                        << (*fault).second->ignored << ")" << std::endl;
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
        _unlatchTimer.show();
        _setTentativeBeamClassTimer.show();
        _setChannelIgnoreTimer.show();
        _evaluateFaultsTimer.show();
        _evaluateIgnoreConditionsTimer.show();
        _mitigateTimer.show();
        _setAllowedBeamClassTimer.show();
        hb.printReport();
        std::cout << "Rate: " << Engine::_rate << " Hz" << std::endl;

        std::cout << "Reload latch: " << Engine::_linacFwLatch << std::endl;
        std::cout << "Reload Config Count: " << Engine::_reloadCount << std::endl;

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

void Engine::showFaultInputs()
{
    if (!isInitialized())
    {
        std::cout << "MPS not initialized - no database" << std::endl;
        return;
    }

    std::cout << "Fault Inputs: " << std::endl;
    std::unique_lock<std::mutex> lock(*_mpsDb->getMutex());

    for (DbFaultInputMap::iterator input = _mpsDb->faultInputs->begin(); input != _mpsDb->faultInputs->end(); ++input)
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
        Firmware::getInstance().setTimingCheckEnable(true);
    }

    Firmware::getInstance().clearAll();


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
            _unlatchAllowed = _unlatchTimer.countdownComplete(1.0);

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

bool Engine::unlatchAllowed()
{
    return _unlatchAllowed;
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

void Engine::startLatchTimeout()
{
    _unlatchTimer.start();
}

void Engine::clearMaxTimers()
{
    hb.clear();
    _checkFaultTime.clear();
    _evaluationCycleTime.clear();
    _mpsDb->clearUpdateTime();
    _unlatchTimer.clear();
    _evaluationCycleTime.clear();
    _setTentativeBeamClassTimer.clear();
    _setChannelIgnoreTimer.clear();
    _evaluateFaultsTimer.clear();
    _evaluateIgnoreConditionsTimer.clear();
    _mitigateTimer.clear();
    _setAllowedBeamClassTimer.clear();
}

long Engine::getAvgWdUpdatePeriod()
{
    return static_cast<long>( hb.getMeanTxPeriod() * 1e6 );
}

long Engine::getMaxWdUpdatePeriod()
{
    return static_cast<long>( hb.getMaxTxPeriod() * 1e6 );
}
