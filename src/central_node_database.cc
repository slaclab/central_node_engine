#include <yaml-cpp/yaml.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/node/emit.h>
#include <yaml-cpp/exceptions.h>

#include <central_node_database_defs.h>
#include <central_node_database_tables.h>
#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_firmware.h>
#include <central_node_engine.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <sys/mman.h>

#include <stdio.h>
#include <log_wrapper.h>

#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
  using namespace easyloggingpp;
  static Logger *databaseLogger;
#endif

extern TimeAverage DeviceInputUpdateTime;
extern TimeAverage AnalogChannelUpdateTime;
extern TimeAverage AppCardDigitalUpdateTime;
extern TimeAverage AppCardAnalogUpdateTime;

std::mutex MpsDb::_mutex;
bool MpsDb::_initialized = false;

MpsDb::MpsDb(uint32_t inputUpdateTimeout)
:
    fwUpdateBuffer( fwUpdateBuferSize, 0 ),
    run( true ),
    // temporarily commented out to debug central_node_database_tst.cc
    // fwUpdateThread( std::thread( &MpsDb::fwUpdateReader, this ) ),
    // fwPCChangeThread( std::thread( &MpsDb::fwPCChangeReader, this ) ),
    // updateInputThread( std::thread( &MpsDb::updateInputs, this ) ),
    // mitigationThread( std::thread( &MpsDb::mitigationWriter, this ) ),
    inputsUpdated(false),
    _fastUpdateTimeStamp(0),
    _diff(0),
    _maxDiff(0),
    _diffCount(0),
    softwareMitigationBuffer(NUM_DESTINATIONS / 8, 0),
    _inputUpdateTime("Input update time", 360),
    _clearUpdateTime(false),
    fwUpdateTimer("FW Update Period", 360),
    _inputUpdateTimeout(inputUpdateTimeout),
    _updateCounter(0),
    _updateTimeoutCounter(0),
    _pcChangeCounter(0),
    _pcChangeBadSizeCounter(0),
    _pcChangeOutOrderCounter(0),
    _pcChangeLossCounter(0),
    _pcChangeSameTagCounter(0),
    _pcChangeFirstPacket(true),
    _pcChangeDebug(false),
    _pcFlagsCounters(Firmware::PcChangePacketFlagsLabels.size(), 0),
    mitigationTxTime( "Mitigation Transmission time", 360 )
{
#if defined(LOG_ENABLED) && !defined(LOG_STDOUT)
  databaseLogger = Loggers::getLogger("DATABASE");
#endif

    std::cout << "Central node database initialized\n"; // temp
    return; // temp

  if (!_initialized) {
    _initialized = true;

    // Initialize the Power class ttansition counters
    for (std::size_t i {0}; i < NUM_DESTINATIONS; ++i)
      for (std::size_t j {0}; j < (1<<POWER_CLASS_BIT_SIZE); ++j)
        _pcCounters[i][j] = 0;

    //  Set thread names
    if ( pthread_setname_np( updateInputThread.native_handle(), "InputUpdates" ) )
      perror( "pthread_setname_np failed for updateInputThread" );

    if ( pthread_setname_np( fwUpdateThread.native_handle(), "FwReader" ) )
        perror( "pthread_setname_np failed for fwUpdateThread" );

    if( pthread_setname_np( mitigationThread.native_handle(), "MitWriter" ) )
        perror( "pthread_setname_np failed for mitigationThread" );

    if( pthread_setname_np( fwPCChangeThread.native_handle(), "PCChange" ) )
        perror( "pthread_setname_np failed for fwPCChangeThread" );
  }
}



MpsDb::~MpsDb() {

  std::cout << "INFO: Stopping update/buffer threads" << std::endl;

  // Stop all threads
  run = false;
  updateInputThread.join();
  std::cout << "INFO: updateInputThread join succeeded" << std::endl;
  mitigationThread.join();
  std::cout << "INFO: mitigationThread join succeeded" << std::endl;
  fwUpdateThread.join();
  std::cout << "INFO: fwUpdateThread join succeeded" << std::endl;

  _inputUpdateTime.show();
  std::cout << "Update counter: " << _updateCounter << std::endl;
}

void MpsDb::unlatchAll() {
    LOG_TRACE("DATABASE", "Unlatching all faults");
    for (DbAnalogChannelMap::iterator it = analogChannels->begin();
         it != analogChannels->end(); ++it) {
      (*it).second->latchedValue = (*it).second->value; // Update value for all threshold bits
    }

    for (DbDeviceInputMap::iterator it = deviceInputs->begin();
         it != deviceInputs->end(); ++it) {
      (*it).second->unlatch();
    }
}
/**
 * This method reads input states from the central node firmware
 * and updates the status of all digital/analog inputs.
 */
void MpsDb::updateInputs()
{
    struct sched_param  param;
    param.sched_priority = 86;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
        perror("sched_setscheduler failed");

    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1)
        perror("mlockall failed");

    std::cout << "Update input thread started" << std::endl;

    for(;;)
    {
        update_buffer_t buffer(fwUpdateBuferSize);
        fwUpdateQueue.pop(buffer);

        {
            std::lock_guard<std::mutex> lock(fwUpdateBufferMutex);
            fwUpdateBuffer = std::move(buffer);
        }

        {
            std::unique_lock<std::mutex> lock(inputsUpdatedMutex);
            while(inputsUpdated)
            {
                inputsUpdatedCondVar.wait_for( lock, std::chrono::milliseconds(5) );
                if(!run)
                {
                    std::cout << "FW Update Data reader interrupted" << std::endl;
                    return;
                }
            }
        }

        uint64_t t;
        memcpy(&t, &fwUpdateBuffer.at(8), sizeof(t));
        uint64_t diff = t - _fastUpdateTimeStamp;
        _fastUpdateTimeStamp = t;

        if (diff > _maxDiff)
            _maxDiff = diff;


        if (diff > 12000000)
            _diffCount++;

        _diff = diff;

        Engine::getInstance()._evaluationCycleTime.start(); // Start timer to measure whole eval cycle

        if (_clearUpdateTime)
        {
            DeviceInputUpdateTime.clear();
            AnalogChannelUpdateTime.clear();
            AppCardDigitalUpdateTime.clear();
            AppCardAnalogUpdateTime.clear();
            _clearUpdateTime = false;
            _inputUpdateTime.clear();
            fwUpdateTimer.clear();
            mitigationTxTime.clear();
            softwareMitigationQueue.clear_counters();
            fwUpdateQueue.clear_counters();
        }

        _inputUpdateTime.start();

        DbApplicationCardMap::iterator applicationCardIt;
        for (applicationCardIt = applicationCards->begin();
            applicationCardIt != applicationCards->end();
            ++applicationCardIt)
        {
            (*applicationCardIt).second->updateInputs();
        }

        _updateCounter++;
        _inputUpdateTime.tick();

        {
            std::lock_guard<std::mutex> lock(inputsUpdatedMutex);
            inputsUpdated = true;
        }
        inputsUpdatedCondVar.notify_one();
    }
}

void MpsDb::configureAllowedClasses()
{
    LOG_TRACE("DATABASE", "Configure: AllowedClasses");
    std::stringstream errorStream;
    uint32_t lowestBeamClassNumber = 100;
    for (DbBeamClassMap::iterator it = beamClasses->begin();
        it != beamClasses->end();
        ++it)
    {
        if ((*it).second->number < lowestBeamClassNumber)
        {
            lowestBeamClassNumber = (*it).second->number;
            lowestBeamClass = (*it).second;
        }
    }

    // Assign BeamClass and BeamDestination to AllowedClass
    for (DbAllowedClassMap::iterator it = allowedClasses->begin();
        it != allowedClasses->end();
        ++it)
    {
        int id = (*it).second->beamClassId;
        DbBeamClassMap::iterator beamIt = beamClasses->find(id);
        if (beamIt == beamClasses->end())
        {
            errorStream <<  "ERROR: Failed to configure database, invalid ID found for BeamClass ("
                << id << ") for AllowedClass (" << (*it).second->id << ")";
            throw(DbException(errorStream.str()));
        }
        (*it).second->beamClass = (*beamIt).second;

        id = (*it).second->beamDestinationId;
        DbBeamDestinationMap::iterator beamDestIt = beamDestinations->find(id);
        if (beamDestIt == beamDestinations->end())
        {
            errorStream << "ERROR: Failed to configure database, invalid ID found for BeamDestinations ("
                << id << ") for AllowedClass (" << (*it).second->id << ")";
            throw(DbException(errorStream.str()));
        }
        (*it).second->beamDestination = (*beamDestIt).second;
    }

    // Iterate through the faultStates and assign the allowed classes through its vector<uint> mitigationIds
    std::vector<unsigned int> mitigationIds;
    for (DbFaultStateMap::iterator faultStateIt = faultStates->begin();
        faultStateIt != faultStates->end(); faultStateIt++) {
        // create allowedClasses map
        if (!(*faultStateIt).second->allowedClasses)
        {
            DbAllowedClassMap *faultAllowedClasses = new DbAllowedClassMap();
            (*faultStateIt).second->allowedClasses = DbAllowedClassMapPtr(faultAllowedClasses);
        }

        // iterate through the faultStateIt->mitigationIds
        mitigationIds = (*faultStateIt).second->mitigationIds;
        for (unsigned int mitigationId : mitigationIds) {
            DbAllowedClassMap::iterator allowedClassIt = allowedClasses->find(mitigationId);
            (*faultStateIt).second->allowedClasses->insert(std::pair<int, DbAllowedClassPtr>((*allowedClassIt).second->id,
            (*allowedClassIt).second));
        }

    }
}

void MpsDb::configureDeviceInputs()
{

    // TODO - Temporarily comment out, may omit this entire function if deviceInputs not needed and replace with configureChannelinputs

    // LOG_TRACE("DATABASE", "Configure: DeviceInputs");
    // std::stringstream errorStream;
    // // Assign DeviceInputs to it's DigitalDevices, and find the Channel
    // // the device is connected to
    // for (DbDeviceInputMap::iterator it = deviceInputs->begin();
    //     it != deviceInputs->end();
    //     ++it)
    // {
    //     int id = (*it).second->digitalDeviceId;

    //     DbDigitalDeviceMap::iterator deviceIt = digitalDevices->find(id);
    //     if (deviceIt == digitalDevices->end())
    //     {
    //         std::cout << "ERROR: Failed to configure database, invalid ID found for DigitalDevice ("
    //             << id << ") for DeviceInput (" << (*it).second->id << ")" << std::endl;
    //         //throw(DbException(errorStream.str()));
    //     }

    //     // Check if digitalDevice is evaluated in firmaware, set deviceInput->fastEvaluation
    //     if ((*deviceIt).second->evaluation == FAST_EVALUATION)
    //         (*it).second->fastEvaluation = true;
    //     else
    //         (*it).second->fastEvaluation = false;

    //     // Create a map to hold deviceInput for the digitalDevice
    //     if (!(*deviceIt).second->inputDevices)
    //     {
    //         DbDeviceInputMap *deviceInputs = new DbDeviceInputMap();
    //         (*deviceIt).second->inputDevices = DbDeviceInputMapPtr(deviceInputs);
    //     }
    //     else
    //     {
    //         // Extra check to make sure the digitalDevice has only one input, if
    //         // it has evaluation set to FAST
    //         if ((*deviceIt).second->evaluation == FAST_EVALUATION)
    //         {
    //             std::cout << "ERROR: Failed to configure database, found DigitalDevice ("
    //                 << id << ") set for FAST_EVALUATION with multiple inputs. Must have single input." << std::endl;
    //             //throw(DbException(errorStream.str()));
    //         }
    //     }
    //     (*deviceIt).second->inputDevices->insert(std::pair<int, DbDeviceInputPtr>((*it).second->id,
    //         (*it).second));
    //     LOG_TRACE("DATABASE", "Adding DeviceInput (" << (*it).second->id << ") to "
    //         " DigitalDevice (" << (*deviceIt).second->id << ")");

    //     int channelId = (*it).second->channelId;
    //     DbAnalogChannelMap::iterator channelIt = digitalChannels->find(channelId);
    //     if (channelIt == digitalChannels->end())
    //     {
    //         std::cout << "ERROR: Failed to configure database, invalid ID found for Channel ("
    //             << channelId << ") for DeviceInput (" << (*it).second->id << ")" << std::endl;
    //         //throw(DbException(errorStream.str()));
    //     }
    //     (*it).second->channel = (*channelIt).second;
    // }


    /* TODO - commented out since new schema doesn't have deviceTypeId. Remove this section once confirmed */

    // // Set the DbDeviceType for all DbDigitaDevices
    // for (DbDigitalDeviceMap::iterator it = digitalDevices->begin();
    //     it != digitalDevices->end();
    //     ++it)
    // {
    //     int typeId = (*it).second->deviceTypeId;

    //     DbDeviceTypeMap::iterator deviceType = deviceTypes->find(typeId);
    //     if (deviceType != deviceTypes->end())
    //     {
    //         (*it).second->deviceType = (*deviceType).second;
    //     }
    //     else
    //     {
    //         errorStream << "ERROR: Failed to configure database, invalid deviceTypeId ("
    //             << typeId << ") for DigitalDevice (" <<  (*it).second->id << ")";
    //         throw(DbException(errorStream.str()));
    //     }
    // }
}

void MpsDb::configureFaultInputs()
{
    LOG_TRACE("DATABASE", "Configure: FaultInputs");
    std::stringstream errorStream;

    // Assign DigitalChannel to FaultInputs
    for (DbFaultInputMap::iterator it = faultInputs->begin();
        it != faultInputs->end();
        ++it)
    {
        int id = (*it).second->channelId;

        // Find the DbDigitalChannel or DbAnalogChannel referenced by the FaultInput
        DbDigitalChannelMap::iterator channelIt = digitalChannels->find(id);
        if (channelIt == digitalChannels->end())
        {
            DbAnalogChannelMap::iterator aChannelIt = analogChannels->find(id);
            if (aChannelIt == analogChannels->end())
            {
                errorStream << "ERROR: Failed to find DigitalChannel/AnalogChannel (" << id
                    << ") for FaultInput (" << (*it).second->id << ")";
                throw(DbException(errorStream.str()));
            }
            // Found the DbAnalogChannel, configure it
            else
            {
                (*it).second->analogChannel = (*aChannelIt).second;

                if ((*aChannelIt).second->evaluation == FAST_EVALUATION)
                {
                    LOG_TRACE("DATABASE", "AnalogChannel " << (*aChannelIt).second->name
                        << ": Fast Evaluation");
                    DbFaultMap::iterator faultIt = faults->find((*it).second->faultId);
                    if (faultIt == faults->end())
                    {
                        errorStream << "ERROR: Failed to find Fault (" << id
                            << ") for FaultInput (" << (*it).second->id << ")";
                        throw(DbException(errorStream.str()));
                    }
                    else
                    {
                        if (!(*faultIt).second->faultStates)
                        {
                            errorStream << "ERROR: No FaultStates found for Fault (" << (*faultIt).second->id
                                << ") for FaultInput (" << (*it).second->id << ")";
                            throw(DbException(errorStream.str()));
                        }

                        // There is one DbFaultState per threshold bit, for BPMs there are
                        // 24 bits (8 for X, 8 for Y and 8 for TMIT). Other analog Channels
                        // have up to 32 bits (4 integrators) - there is one destination mask
                        // per integrator

                        // There is a unique power class for each threshold bit (each DbFaultState)
                        // The destination masks for the thresholds for the same integrator
                        // must be and'ed together.

                        // Go through the DbFaultStates for this DbFault, i.e. the individual threshold bits
                        for (DbFaultStateMap::iterator faultState = (*faultIt).second->faultStates->begin();
                            faultState != (*faultIt).second->faultStates->end();
                            ++faultState)
                        {
                            // The DbDeviceState value defines which integrator the fault belongs to.
                            // Bits 0 through 7 are for the first integrator, 8 through 15 for the second,
                            // and so on.
                            uint32_t value = (*faultState).second->value;
                            int integratorIndex = 4;

                            if (value & 0x000000FF) integratorIndex = 0;
                            if (value & 0x0000FF00) integratorIndex = 1;
                            if (value & 0x00FF0000) integratorIndex = 2;
                            if (value & 0xFF000000) integratorIndex = 3;

                            if (integratorIndex >= 4)
                            {
                                errorStream << "ERROR: Invalid value Fault (" << (*faultIt).second->id
                                    << "), FaultInput (" << (*it).second->id << "), value=0x"
                                    << std::hex << (*faultState).second->value << std::dec;
                                throw(DbException(errorStream.str()));
                            }

                            uint32_t thresholdIndex = 0;
                            bool isSet = false;
                            while (!isSet)
                            {
                                if (value & 0x01)
                                {
                                    isSet = true;
                                }
                                else
                                {
                                    thresholdIndex++;
                                    value >>= 1;
                                }

                                if (thresholdIndex >= sizeof(uint32_t) * 8)
                                {
                                    errorStream << "ERROR: Invalid threshold bit for Fault (" << (*faultIt).second->id
                                        << "), FaultInput (" << (*it).second->id << ")";
                                    throw(DbException(errorStream.str()));
                                }
                            }

                            // Find the power classes for analog thresholds based on the AllowedClasses.
                            // Note: even though the database allows for different power classes for
                            // different destinations for the same fault, the firmware only has a single
                            // power class to apply for a 16-bit destination mask. For example:
                            // IM01B Charge Fault has one AllowedClass per destination, where
                            //   Shutter = power class 1 (destination 1)
                            //   AOM = power class 0 (destination 2)
                            // The power class for the destination mask 0x03 is power class 0 (the lowest)
                            for (DbAllowedClassMap::iterator allowedClass = (*faultState).second->allowedClasses->begin();
                                allowedClass != (*faultState).second->allowedClasses->end();
                                ++allowedClass)
                            {
                                (*aChannelIt).second->fastDestinationMask[integratorIndex] |=
                                    (*allowedClass).second->beamDestination->destinationMask;
                                LOG_TRACE("DATABASE", "PowerClass: integrator=" << integratorIndex
                                    << " threshold index=" << thresholdIndex
                                    << " current power=" << (*aChannelIt).second->fastPowerClass[thresholdIndex]
                                    << " power=" << (*allowedClass).second->beamClass->number
                                    << " allowedClassId=" << (*allowedClass).second->id
                                    << " destinationMask=0x" << std::hex << (*allowedClass).second->beamDestination->destinationMask << std::dec );

                                if ((*aChannelIt).second->fastPowerClassInit[thresholdIndex] == 1)
                                {
                                    (*aChannelIt).second->fastPowerClass[thresholdIndex] =
                                    (*allowedClass).second->beamClass->number;
                                    (*aChannelIt).second->fastPowerClassInit[thresholdIndex] = 0;
                                }
                                else
                                {
                                    if ((*allowedClass).second->beamClass->number <
                                        (*aChannelIt).second->fastPowerClass[thresholdIndex])
                                    {
                                        (*aChannelIt).second->fastPowerClass[thresholdIndex] =
                                        (*allowedClass).second->beamClass->number;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            (*it).second->digitalChannel = (*channelIt).second;
            // If the DbDigitalChannel is set for fast evaluation, save a pointer to the
            // DbFaultInput
            if ((*channelIt).second->evaluation == FAST_EVALUATION)
            {
                DbFaultMap::iterator faultIt = faults->find((*it).second->faultId);
                if (faultIt == faults->end())
                {
                    errorStream << "ERROR: Failed to find Fault (" << id
                        << ") for FaultInput (" << (*it).second->id << ")";
                    throw(DbException(errorStream.str()));
                }
                else
                {
                    if (!(*faultIt).second->faultStates)
                    {
                        errorStream << "ERROR: No FaultStates found for Fault (" << (*faultIt).second->id
                            << ") for FaultInput (" << (*it).second->id << ")";
                        throw(DbException(errorStream.str()));
                    }
                    (*channelIt).second->fastDestinationMask = 0;
                    (*channelIt).second->fastPowerClass = 100;
                    (*channelIt).second->fastExpectedState = 0;

                    if ((*faultIt).second->faultStates->size() != 1)
                    {
                        errorStream << "ERROR: DigitalChannel configured with FAST evaluation must have one fault state only."
                            << " Found " << (*faultIt).second->faultStates->size() << " fault states for "
                            << "channel " << (*channelIt).second->name;
                        throw(DbException(errorStream.str()));
                    }
                    DbFaultStateMap::iterator faultState = (*faultIt).second->faultStates->begin();

                    // Set the expected state as the oposite of the expected faulted value
                    // For example a faulted fast valve sets the digital input to high (0V, digital 0),
                    // the expected normal state is 1.
                    if (!(*faultState).second->value)
                      (*channelIt).second->fastExpectedState = 1;

                    //    DbAllowedClassMap::iterator allowedClass = (*faultState).second->allowedClasses->begin();
                    for (DbAllowedClassMap::iterator allowedClass = (*faultState).second->allowedClasses->begin();
                        allowedClass != (*faultState).second->allowedClasses->end();
                        ++allowedClass)
                    {
                        (*channelIt).second->fastDestinationMask |= (*allowedClass).second->beamDestination->destinationMask;
                        if ((*allowedClass).second->beamClass->number < (*channelIt).second->fastPowerClass)
                            (*channelIt).second->fastPowerClass = (*allowedClass).second->beamClass->number;

                    }
                }
            }
        }
    }
    //  std::cout << "assigning fault inputs to faults" << std::endl;

    // Assing fault inputs to faults
    for (DbFaultInputMap::iterator it = faultInputs->begin();
        it != faultInputs->end();
        ++it)
    {
        int id = (*it).second->faultId;

        DbFaultMap::iterator faultIt = faults->find(id);
        if (faultIt == faults->end())
        {
            errorStream <<  "ERROR: Failed to configure database, invalid ID found for Fault ("
                << id << ") for FaultInput (" << (*it).second->id << ")";
            throw(DbException(errorStream.str()));
        }

        // Create a map to hold faultInputs for the fault
        if (!(*faultIt).second->faultInputs)
        {
            DbFaultInputMap *faultInputs = new DbFaultInputMap();
            (*faultIt).second->faultInputs = DbFaultInputMapPtr(faultInputs);
        }
        (*faultIt).second->faultInputs->insert(std::pair<int, DbFaultInputPtr>((*it).second->id,
            (*it).second));
    }

    //  std::cout << "assign evaluation" << std::endl;

    // Assign an evaluation to a Fault based on the inputs to its FaultInputs
    // Faults whose inputs are all evaluated in firmware should be only handled
    // by the firmware.
    // TODO: set DbFault.evaluation
    for (DbFaultMap::iterator faultIt = faults->begin(); faultIt != faults->end(); ++faultIt)
    {
        bool slowEvaluation = false;
        if (!(*faultIt).second->faultInputs)
        {
            errorStream << "ERROR: Missing faultInputs map for Fault \""
                << (*faultIt).second->name << "\": "
                << (*faultIt).second->pv;
            throw(DbException(errorStream.str()));
        }

        for (DbFaultInputMap::iterator inputIt = (*faultIt).second->faultInputs->begin();
            inputIt != (*faultIt).second->faultInputs->end();
            ++inputIt)
        {
            if ((*inputIt).second->analogChannel)
            {
                if ((*inputIt).second->analogChannel->evaluation == SLOW_EVALUATION)
                    slowEvaluation = true;
            }
            else if ((*inputIt).second->digitalChannel)
            {
                if ((*inputIt).second->digitalChannel->evaluation == SLOW_EVALUATION)
                    slowEvaluation = true;
            }
        }

        if (slowEvaluation)
            (*faultIt).second->evaluation = SLOW_EVALUATION;
        else
            (*faultIt).second->evaluation = FAST_EVALUATION;
    }
    //  std::cout << "done with faultInputs" << std::endl;
}

void MpsDb::configureFaultStates()
{

    LOG_TRACE("DATABASE", "Configure: FaultStates");
    std::stringstream errorStream;

    // Assign all FaultStates to a Fault, and also
    // find the DeviceState and save a reference in the FaultState
    for (DbFaultStateMap::iterator it = faultStates->begin();
        it != faultStates->end();
        ++it)
    {
        int id = (*it).second->faultId;

        DbFaultMap::iterator faultIt = faults->find(id);
        if (faultIt == faults->end())
        {
            errorStream << "ERROR: Failed to configure database, invalid ID found for Fault ("
                << id << ") for FaultState (" << (*it).second->id << ")";
            throw(DbException(errorStream.str()));
        }

        // Create a map to hold faultInputs for the fault
        if (!(*faultIt).second->faultStates)
        {
            DbFaultStateMap *digFaultStates = new DbFaultStateMap();
            (*faultIt).second->faultStates = DbFaultStateMapPtr(digFaultStates);
        }
        (*faultIt).second->faultStates->insert(std::pair<int, DbFaultStatePtr>((*it).second->id,
            (*it).second));
        LOG_TRACE("DATABASE", "Adding FaultState (" << (*it).second->id << ") to "
            " Fault (" << (*faultIt).second->id << ", " << (*faultIt).second->name
            << ", " << (*faultIt).second->description << ")");

        // If this DigitalFault is the default, then assign it to the fault:
        if ((*it).second->defaultState && !((*faultIt).second->defaultFaultState))
            (*faultIt).second->defaultFaultState = (*it).second;
    }

    // After assigning all FaultStates to Faults, check if all Faults
    // do have FaultStates
    for (DbFaultMap::iterator fault = faults->begin();
        fault != faults->end();
        ++fault)
    {
        if (!(*fault).second->faultStates)
        {
            errorStream << "ERROR: Fault " << (*fault).second->name << " ("
                << (*fault).second->pv << ", id="
                << (*fault).second->id << ") has no FaultStates";
            throw(DbException(errorStream.str()));
        }
    }
}

/* Assign the corresponding application card type to the analog channel */
void MpsDb::configureAnalogChannels()
{
    LOG_TRACE("DATABASE", "Configure: AnalogChannels");
    std::stringstream errorStream;

    for (DbAnalogChannelMap::iterator it = analogChannels->begin();
        it != analogChannels->end();
        ++it)
    {
        int cardId = (*it).second->cardId;
        
        DbApplicationCardMap::iterator appCardIt = applicationCards->find(cardId);
        int cardTypeId = (*appCardIt).second->applicationTypeId;

        DbApplicationTypeMap::iterator cardTypeIt = applicationTypes->find(cardTypeId);
        if (cardTypeIt != applicationTypes->end())
        {
            (*it).second->appType = (*cardTypeIt).second;
        }
        else
        {
            errorStream << "ERROR: Failed to configure database, invalid cardTypeId ("
                << cardTypeId << ") for ApplicationCard (" <<  (*it).second->id << ")";
            throw(DbException(errorStream.str()));
        }
    }
}

/* Assign the corresponding faults and digital channel to the ignore condition */
void MpsDb::configureIgnoreConditions()
{
    LOG_TRACE("DATABASE", "Configure: IgnoreConditions");
    std::stringstream errorStream;

    // Loop through the Faults->ignoreConditions, and assign to corresponding IgnoreCondition
    std::vector<unsigned int> ignoreConditionIds;
    for (DbFaultMap::iterator faultIt = faults->begin();
        faultIt != faults->end();
        faultIt++)
    {
        // iterate through the faultIt->ignoreConditionIds
        ignoreConditionIds = (*faultIt).second->ignoreConditionIds;
        for (unsigned int ignoreConditionId : ignoreConditionIds) {
            DbIgnoreConditionMap::iterator ignoreConditionIt = ignoreConditions->find(ignoreConditionId);
            // Initialize DbFaultMapPtr of ignoreCondition if haven't already
            if (!(*ignoreConditionIt).second->faults)
            {
                DbFaultMap *faults = new DbFaultMap();
                (*ignoreConditionIt).second->faults = DbFaultMapPtr(faults);
            }
            (*ignoreConditionIt).second->faults->insert(std::pair<int, DbFaultPtr>((*faultIt).second->id,
            (*faultIt).second));
        }

    }

    // Loop through IgnoreConditions, and assign the corresponding digital channel
    for (DbIgnoreConditionMap::iterator ignoreConditionIt = ignoreConditions->begin();
        ignoreConditionIt != ignoreConditions->end();
        ignoreConditionIt++)
    {
        DbDigitalChannelMap::iterator digitalChannelIt = digitalChannels->find((*ignoreConditionIt).second->digitalChannelId);
        (*ignoreConditionIt).second->digitalChannel = (*digitalChannelIt).second;
    }

    // TODO - Check if we need to create FK reference to faultState - it seems in the engine logic it does but how can we do it without faultStateId?

}

/**
 * Configure each application card, setting its map of digital or analog
 * inputs. Each application card gets a pointer to the buffer containing
 * the fast rules (sent to firmware) and the buffer containing the
 * input updates received from firmware at 360 Hz.
 */
void MpsDb::configureApplicationCards()
{
    LOG_TRACE("DATABASE", "Configure: ApplicationCards");
    std::stringstream errorStream;

    // Set the address of the firmware configuration location for each application card; and
    // the address of the firmware input update location for each application card
    uint8_t *configBuffer = 0;
    // uint8_t *updateBuffer = 0;
    for (DbApplicationCardMap::iterator applicationCardIt = applicationCards->begin();
        applicationCardIt != applicationCards->end();
        ++applicationCardIt)
    {
        DbApplicationCardPtr aPtr = (*applicationCardIt).second;

        // Find its crate
        DbCrateMap::iterator crateIt;
        crateIt = crates->find(aPtr->crateId);
        if (crateIt != crates->end())
            aPtr->crate = (*crateIt).second;

        // Configuration buffer
        configBuffer = fastConfigurationBuffer + aPtr->globalId * APPLICATION_CONFIG_BUFFER_SIZE_BYTES;
        aPtr->applicationConfigBuffer = reinterpret_cast<ApplicationConfigBufferBitSet *>(configBuffer);

        // Update buffer
        // updateBuffer = fwUpdateBuffer.getReadPtr()->at(
        //   APPLICATION_UPDATE_BUFFER_HEADER_SIZE_BYTES + // Skip header (timestamp + zeroes)
        //   aPtr->globalId * APPLICATION_UPDATE_BUFFER_INPUTS_SIZE_BYTES); // Jump to correct area according to the globalId

        // For debugging purposes only
        aPtr->applicationUpdateBufferFull = reinterpret_cast<ApplicationUpdateBufferFullBitSet *>(&fwUpdateBuffer);

        // New mapping
        aPtr->setUpdateBufferPtr(&fwUpdateBuffer);

        // Find the ApplicationType for each card
        DbApplicationTypeMap::iterator applicationTypeIt = applicationTypes->find(aPtr->applicationTypeId);
        if (applicationTypeIt != applicationTypes->end())
            aPtr->applicationType = (*applicationTypeIt).second;
        
        LOG_TRACE("DATABASE", "AppCard [" << aPtr->globalId << ", " << aPtr->name << "] config/update buffer alloc");
    }

    // Get all devices for each application card, starting with the digital devices
    // Create a map of digital devices for each DbApplicationCard
    for (DbDigitalChannelMap::iterator digitalChannelIt = digitalChannels->begin();
        digitalChannelIt != digitalChannels->end();
        ++digitalChannelIt)
    {
        DbDigitalChannelPtr digitalChannelPtr = (*digitalChannelIt).second;
        DbApplicationCardMap::iterator applicationCardIt = applicationCards->find(digitalChannelPtr->cardId);
        if (applicationCardIt != applicationCards->end())
        {
            DbApplicationCardPtr aPtr = (*applicationCardIt).second;
            // Alloc a new map for digital devices if one is not there yet
            if (!aPtr->digitalChannels)
            {
                DbDigitalChannelMap *digitalChannels = new DbDigitalChannelMap();
                aPtr->digitalChannels = DbDigitalChannelMapPtr(digitalChannels);
            }
            // Once the map is there, add the digital device
            aPtr->digitalChannels->insert(std::pair<int, DbDigitalChannelPtr>(digitalChannelPtr->id, digitalChannelPtr));
            LOG_TRACE("DATABASE", "AppCard [" << aPtr->globalId << ", " << aPtr->name << "], DigitalChannel: " << digitalChannelPtr->name);
        }
    }

    // Do the same for analog channels
    for (DbAnalogChannelMap::iterator analogChannelIt = analogChannels->begin();
        analogChannelIt != analogChannels->end();
        ++analogChannelIt)
    {
        DbAnalogChannelPtr analogChannelPtr = (*analogChannelIt).second;
        DbApplicationCardMap::iterator applicationCardIt = applicationCards->find(analogChannelPtr->cardId);
        if (applicationCardIt != applicationCards->end())
        {
            DbApplicationCardPtr aPtr = (*applicationCardIt).second;
            // Alloc a new map for analog devices if one is not there yet
            if (aPtr->digitalChannels)
            {
                errorStream << "ERROR: Found ApplicationCard with digital AND analog devices,"
                    << " can't handle that (cardId="
                    << (*analogChannelIt).second->cardId << ")";
                throw(DbException(errorStream.str()));
            }

            if (!aPtr->analogChannels)
            {
                DbAnalogChannelMap *analogChannels = new DbAnalogChannelMap();
                aPtr->analogChannels = DbAnalogChannelMapPtr(analogChannels);
            }

            // Once the map is there, add the analog device
            aPtr->analogChannels->insert(std::pair<int, DbAnalogChannelPtr>(analogChannelPtr->id, analogChannelPtr));
            LOG_TRACE("DATABASE", "AppCard [" << aPtr->globalId << ", " << aPtr->name << "], AnalogChannel: " << analogChannelPtr->name);
            analogChannelPtr->numChannelsCard = aPtr->applicationType->analogChannelCount;
        }
    }
    std::cout << "Done!" << std::endl;

    for (DbApplicationCardMap::iterator applicationCardIt = applicationCards->begin();
        applicationCardIt != applicationCards->end();
        ++applicationCardIt)
    {
        (*applicationCardIt).second->configureUpdateBuffers();
    }

    // Deal with special case where one device input is not in same application card.
    for (DbDigitalChannelMap::iterator digitalChannelIt = digitalChannels->begin();
        digitalChannelIt != digitalChannels->end();
        ++digitalChannelIt)
    {
        DbDigitalChannelPtr digitalChannelPtr = (*digitalChannelIt).second;
        if (digitalChannelPtr->inputDevices) {
            for (DbDeviceInputMap::iterator deviceInput = digitalChannelPtr->inputDevices->begin();
                    deviceInput != digitalChannelPtr->inputDevices->end(); ++deviceInput) {
                if (!(*deviceInput).second->configured) {
                    uint32_t diCard = (*deviceInput).second->channel->cardId;
                    DbApplicationCardMap::iterator applicationCardIt = applicationCards->find(diCard);
                    if (applicationCardIt != applicationCards->end())
                    {
                        DbApplicationCardPtr aPtr = (*applicationCardIt).second;
                        std::vector<uint8_t>* buff = aPtr->getFwUpdateBuffer();
                        size_t                lowBufOff = aPtr->getWasLowBufferOffset();
                        size_t                highBufOff = aPtr->getWasHighBufferOffset();
                        (*deviceInput).second->setUpdateBuffers(buff, lowBufOff, highBufOff);
                        (*deviceInput).second->configured = true;
                        std::cout << "INFO: Device Input for " << digitalChannelPtr->name << " configured!" << std::endl;
                    }
                }
            }
        }
    }
    std::cout << "End of the line" << std::endl;
}

void MpsDb::configureBeamDestinations()
{
    std::cout << "BeamDestinations: ";
    for (DbBeamDestinationMap::iterator it = beamDestinations->begin();
        it != beamDestinations->end();
        ++it)
    {
        std::cout << (*it).second->name << " ";
        (*it).second->setSoftwareMitigationBuffer( &softwareMitigationBuffer );
        (*it).second->previousAllowedBeamClass = lowestBeamClass;
        (*it).second->allowedBeamClass = lowestBeamClass;
    }
    std::cout << std::endl;
}

void MpsDb::clearMitigationBuffer()
{
    mit_buffer_t( NUM_DESTINATIONS/8, 0 ).swap(softwareMitigationBuffer);
}

/**
 * After the YAML database file is loaded this method must be called to resolve
 * references between tables. In the YAML table entries there is a reference to
 * another table using a 'foreign key'. The configure*() methods look for the
 * actual item referenced by the foreign key and saves a pointer to the
 * referenced element for direct access. This allows the engine to evaluate
 * faults without having to search for entries.
 */
void MpsDb::configure()
{
    configureAllowedClasses();
    // configureDeviceInputs(); Temporarily commented out, TODO - See if this is still needed for new schema
    configureFaultStates();
    configureAnalogChannels();
    configureFaultInputs();
    configureIgnoreConditions();
    configureApplicationCards();
    // configureBeamDestinations();
}

void MpsDb::forceBeamDestination(uint32_t beamDestinationId, uint32_t beamClassId)
{
    DbBeamDestinationMap::iterator beamDestIt = beamDestinations->find(beamDestinationId);
    if (beamDestIt != beamDestinations->end())
    {
        if (beamClassId != CLEAR_BEAM_CLASS)
        {
            DbBeamClassMap::iterator beamClassIt = beamClasses->find(beamClassId);
            if (beamClassIt != beamClasses->end())
            {
                (*beamDestIt).second->setForceBeamClass((*beamClassIt).second);
            }
            else
            {
                (*beamDestIt).second->resetForceBeamClass();
            }
        }
        else
        {
            (*beamDestIt).second->resetForceBeamClass();
        }
    }
}

void MpsDb::softPermitDestination(uint32_t beamDestinationId, uint32_t beamClassId)
{
    DbBeamDestinationMap::iterator beamDestIt = beamDestinations->find(beamDestinationId);
    if (beamDestIt != beamDestinations->end())
    {
        if (beamClassId != CLEAR_BEAM_CLASS)
        {
            DbBeamClassMap::iterator beamClassIt = beamClasses->find(beamClassId);
            if (beamClassIt != beamClasses->end())
            {
                (*beamDestIt).second->setSoftPermit((*beamClassIt).second);
            }
            else
            {
                (*beamDestIt).second->resetSoftPermit();
            }
        }
        else
        {
            (*beamDestIt).second->resetSoftPermit();
        }
    }
}
//This function to be used for 100 MeV operation and then removed - it will give one more hook to force BC to be 120 Hz MAX
void MpsDb::setMaxPermit(uint32_t beamClassId)
{

    for (DbBeamDestinationMap::iterator beamDestIt = beamDestinations->begin();
         beamDestIt != beamDestinations->end();
         ++beamDestIt)
    {
      if ((*beamDestIt).second->name != "LASER")
      {
        if (beamDestIt != beamDestinations->end())
        {
            if (beamClassId != CLEAR_BEAM_CLASS)
            {
                DbBeamClassMap::iterator beamClassIt = beamClasses->find(beamClassId);
                if (beamClassIt != beamClasses->end())
                {
                    (*beamDestIt).second->setMaxPermit((*beamClassIt).second);
                }
                else
                {
                    (*beamDestIt).second->resetMaxPermit();
                }
            }
            else
            {
                (*beamDestIt).second->resetMaxPermit();
            }
        }
      }
    }
}

void MpsDb::writeFirmwareConfiguration(bool enableTimeout)
{
    // Write configuration for each application in the system
    LOG_TRACE("DATABASE", "Writing config to firmware, num applications: " << applicationCards->size());
    int i = 0;
    for (DbApplicationCardMap::iterator card = applicationCards->begin();
        card != applicationCards->end();
        ++card)
    {
        (*card).second->writeConfiguration(enableTimeout);
        Firmware::getInstance().writeConfig((*card).second->globalId, fastConfigurationBuffer +
            (*card).second->globalId * APPLICATION_CONFIG_BUFFER_SIZE_BYTES,
            APPLICATION_CONFIG_BUFFER_USED_SIZE_BYTES);
        i++;
    }

    // If the app timeout were set to enable, write the configuration to FW
    // after looping over all applications in the system
    if (enableTimeout)
        Firmware::getInstance().writeAppTimeoutMask();

    // Write the timing verifying parameters for each beam class
    uint32_t time[FW_NUM_BEAM_CLASSES];
    uint32_t period[FW_NUM_BEAM_CLASSES];
    uint32_t charge[FW_NUM_BEAM_CLASSES];

    for (uint32_t i = 0; i < FW_NUM_BEAM_CLASSES; ++i)
    {
        time[i] = 1;
        period[i] = 0;
        charge[i] = 4294967295;
    }

    for (DbBeamClassMap::iterator beamClass = beamClasses->begin();
        beamClass != beamClasses->end();
        ++beamClass)
    {
        time[(*beamClass).second->number] = (*beamClass).second->integrationWindow;
        period[(*beamClass).second->number] = (*beamClass).second->minPeriod;
        charge[(*beamClass).second->number] = (*beamClass).second->totalCharge;
    }

    Firmware::getInstance().writeTimingChecking(time, period, charge);

    // Firmware command to actually switch to the new configuration
    Firmware::getInstance().switchConfig();
}

void MpsDb::setName(std::string yamlFileName)
{
    name = yamlFileName;
}

/**
 * The MPS database YAML file is composed of several "tables"/"set of entries".
 * This method opens each table as a YAML::Node and loads all entry contents
 * into the MpsDb maps. Each maps uses the MPS database id as key.
 */
int MpsDb::load(std::string yamlFileName)
{
    std::stringstream errorStream;
    YAML::Node doc;
    std::vector<YAML::Node> nodes;

    LOG_TRACE("DATABASE", "Loading YAML from file " << yamlFileName);
    try
    {
        nodes = YAML::LoadAllFromFile(yamlFileName);
    }
    catch (YAML::BadFile &e)
    {
        errorStream << "ERROR: Please check if YAML file is readable";
        throw(DbException(errorStream.str()));
    }
    catch (...)
    {
        errorStream << "ERROR: Failed to load YAML file ("
            << yamlFileName << ")";
        throw(DbException(errorStream.str()));
    }

    LOG_TRACE("DATABASE", "Parsing YAML");
    for (std::vector<YAML::Node>::iterator node = nodes.begin();
        node != nodes.end();
        ++node)
    {
        std::string s = YAML::Dump(*node);
        size_t found = s.find(":");
        if (found == std::string::npos)
        {
            errorStream << "ERROR: Can't find Node name in YAML file ("
                << yamlFileName << ")";
            throw(DbException(errorStream.str()));
        }
        std::string nodeName = s.substr(0, found);

        std::cout << nodeName << std::endl; // TEMP

        LOG_TRACE("DATABASE", "Parsing \"" << nodeName << "\"");

        if (nodeName == "Crate")
        {
            crates = (*node).as<DbCrateMapPtr>();
        }
        else if (nodeName == "ApplicationType")
        {
            applicationTypes = (*node).as<DbApplicationTypeMapPtr>();
        }
        else if (nodeName == "ApplicationCard")
        {
            applicationCards = (*node).as<DbApplicationCardMapPtr>();
        }
        else if (nodeName == "DigitalChannel")
        {
            digitalChannels = (*node).as<DbDigitalChannelMapPtr>();
        }
        else if (nodeName == "AnalogChannel")
        {
            analogChannels = (*node).as<DbAnalogChannelMapPtr>();
        }
        // TODO - Temporarily commented out until confirmed to delete
        // else if (nodeName == "DeviceType")
        // {
        //     deviceTypes = (*node).as<DbDeviceTypeMapPtr>();
        // }
        // else if (nodeName == "DeviceState")
        // {
        //     deviceStates = (*node).as<DbDeviceStateMapPtr>();
        // }
        // else if (nodeName == "DeviceInput")
        // {
        //     deviceInputs = (*node).as<DbDeviceInputMapPtr>();
        // }
        else if (nodeName == "Fault")
        {
            faults = (*node).as<DbFaultMapPtr>();
        }
        else if (nodeName == "FaultInput")
        {
            faultInputs = (*node).as<DbFaultInputMapPtr>();
        }
        else if (nodeName == "FaultState")
        {
            faultStates = (*node).as<DbFaultStateMapPtr>();
        }
        else if (nodeName == "BeamDestination")
        {
            beamDestinations = (*node).as<DbBeamDestinationMapPtr>();
        }
        else if (nodeName == "Mitigation")
        {
            allowedClasses = (*node).as<DbAllowedClassMapPtr>();
        }
        else if (nodeName == "BeamClass")
        {
            beamClasses = (*node).as<DbBeamClassMapPtr>();
        }
        else if (nodeName == "AllowedClass")
        {
            allowedClasses = (*node).as<DbAllowedClassMapPtr>();
        }
        else if (nodeName == "IgnoreCondition")
        {
            ignoreConditions = (*node).as<DbIgnoreConditionMapPtr>();
        }
        else if (nodeName == "DatabaseInfo")
        {
            databaseInfo = (*node).as<DbInfoMapPtr>();
        }
        else
        {
            // Do not raise exception if the YAML node is known, but
            // does not need to be loaded by central node engine
            if (nodeName != "LinkNode")
            {
                errorStream << "ERROR: Unknown YAML node name ("
                    << nodeName << ")";
                throw(DbException(errorStream.str()));
            }
        }
    }

    setName(yamlFileName);

    // Zero out the buffer that holds the firmware configuration
    //  memset(fastConfigurationBuffer, 0, NUM_APPLICATIONS * APPLICATION_CONFIG_BUFFER_SIZE);

    return 0;
}

/**
 * Print out the digital/analog inputs (DbFaultInput and DbAnalogChannels)
 */
void MpsDb::showFaults()
{
    std::cout << "+-------------------------------------------------------" << std::endl;
    std::cout << "| Faults: " << std::endl;
    std::cout << "+-------------------------------------------------------" << std::endl;
    std::unique_lock<std::mutex> lock(_mutex);
    for (DbFaultMap::iterator fault = faults->begin();
        fault != faults->end();
        ++fault)
    {
        showFault((*fault).second);
    }
    std::cout << "+-------------------------------------------------------" << std::endl;
}

void MpsDb::showFastUpdateBuffer(uint32_t begin, uint32_t size)
{
    // Make a (thread-safe) copy of the buffer, instead of holding
    // the mutex while iterating over the buffer.
    std::vector<uint8_t> buffer( getFastUpdateBuffer() );

    for (uint32_t address = begin; address < begin + size; ++address)
    {
        if (address % 16 == 0)
        {
            std::cout << std::endl;
            std::cout << std::setw(5) << std::hex << address << std::dec << " ";
        }

        std::cout << " ";

        char c1 = buffer.at(address) & 0x0F;
        char c2 = (buffer.at(address) & 0xF0) >> 4;

        if (c1 <= 9)
            c1 += 0x30;
        else
            c1 = 0x61 + (c1 - 10);

        if (c2 <= 9)
            c2 += 0x30;
        else
            c2 = 0x61 + (c2 - 10);

        std::cout << c1 << c2;
    }
}

void MpsDb::showFault(DbFaultPtr fault)
{
    std::cout << "| " << fault->name << std::endl;
    if (!fault->faultInputs)
    {
        std::cout << "| - WARNING: No inputs to this fault" << std::endl;
    }
    else
    {
        for (DbFaultInputMap::iterator input = fault->faultInputs->begin();
            input != fault->faultInputs->end();
            ++input)
        {
            int channelId = (*input).second->channelId;
            DbDigitalChannelMap::iterator digitalChannel = digitalChannels->find(channelId);
            if (digitalChannel == digitalChannels->end())
            {
                // If no inputs, then this could be an analog channel
                DbAnalogChannelMap::iterator analogChannel = analogChannels->find(channelId);
                if (analogChannel == analogChannels->end())
                {
                    std::cout << "| - WARNING: No digital/analog devices for this fault!";
                }
                else
                {
                    std::cout << "| - Input[" << (*analogChannel).second->name
                        << "], Bypass[";
                    if (!(*analogChannel).second->bypass)
                    {
                        std::cout << "WARNING: NO BYPASS INFO]";
                    }
                    else
                    {
                        for (uint32_t i = 0; i < ANALOG_DEVICE_NUM_THRESHOLDS; ++i)
                        {
                            if ((*analogChannel).second->bypass[i]->status == BYPASS_VALID)
                            {
                                std::cout << "V";
                            }
                            else
                            {
                                std::cout << "E";
                            }
                        }
                        std::cout << "]";
                    }
                    std::cout << std::endl;
                }
            }
            else
            {
                for (DbDeviceInputMap::iterator devInput =
                    (*digitalChannel).second->inputDevices->begin();
                devInput != (*digitalChannel).second->inputDevices->end(); ++devInput)
                {
                    std::cout << "| - Input[" << (*devInput).second->id
                        << "], Position[" << (*devInput).second->bitPosition
                        << "], Bypass[";

                    if (!(*devInput).second->bypass)
                    {
                        std::cout << "WARNING: NO BYPASS INFO]";
                    }
                    else
                    {
                        if ((*devInput).second->bypass->status == BYPASS_VALID)
                            std::cout << "VALID]";
                        else
                            std::cout << "EXPIRED]";
                    }
                    std::cout << std::endl;
                }
            }
        }
    }
}

void MpsDb::showMitigation()
{
    std::unique_lock<std::mutex> lock(_mutex);

    std::cout << "Allowed power classes at beam destinations:" << std::endl;
    for (DbBeamDestinationMap::iterator mitDevice = beamDestinations->begin();
        mitDevice != beamDestinations->end();
        ++mitDevice)
    {
        std::cout << "  " << (*mitDevice).second->name << ": " << (*mitDevice).second->allowedBeamClass->number << std::endl;
    }
}

void MpsDb::showInfo()
{
    std::unique_lock<std::mutex> lock(_mutex);

    std::cout << "Current database information:" << std::endl;
    std::cout << "File: " << name << std::endl;
    std::cout << "Update counter: " << _updateCounter << std::endl;
    std::cout << "Input update timeout " << _inputUpdateTimeout << " usec" << std::endl;
    std::cout << "Total devices configured: " << getTotalDeviceCount() << std::endl;
    printPCChangeInfo();

    printMap<DbInfoMapPtr, DbInfoMap::iterator>
        (std::cout, databaseInfo, "DatabaseInfo");

    _inputUpdateTime.show();
    fwUpdateTimer.show();
    mitigationTxTime.show();
    AnalogChannelUpdateTime.show();
    DeviceInputUpdateTime.show();
    AppCardDigitalUpdateTime.show();
    AppCardAnalogUpdateTime.show();

    std::cout << "Max TimeStamp diff    : " << _maxDiff << std::endl;
    _maxDiff = 0;
    std::cout << "Current TimeStamp diff: " << _diff << std::endl;
    std::cout << "Diff > 12ms count     : " << _diffCount << std::endl;
    std::cout << "Update timout counter : " << _updateTimeoutCounter << std::endl;
    std::cout << "Mit. Queue max size   : " << softwareMitigationQueue.get_max_size() << std::endl;
    std::cout << "Update Queue max size : " << fwUpdateQueue.get_max_size() << std::endl;
}

void MpsDb::printPCChangeLastPacketInfo() const
{
    std::cout << "Tag        : "   << _pcChangeTag << std::endl;
    std::cout << "Flags      : 0x" << std::setw(4) << std::setfill('0') << std::hex << unsigned(_pcChangeFlags) << std::dec << std::endl;
    std::cout << "Timestamp  : "   << _pcChangeTimeStamp << std::endl;
    std::cout << "PowerClass : 0x" << std::setw(16) << std::setfill('0') << std::hex << _pcChangePowerClass << std::dec << std::endl;
}

void MpsDb::printPCChangeInfo() const
{
    std::cout << "Power Class Change Messages Info:" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    std::cout << "- Number of valid packet received        : " << _pcChangeCounter << std::endl;
    std::cout << "- Number of lost packets                 : " << _pcChangeLossCounter << std::endl;
    std::cout << "- Number of packet with bad sizes        : " << _pcChangeBadSizeCounter << std::endl;
    std::cout << "- Number of out-of-order packets         : " << _pcChangeOutOrderCounter << std::endl;
    std::cout << "- Number of packet with same tag         : " << _pcChangeSameTagCounter << std::endl;
    std::cout << "- Flag error counters:" << std::endl;
    auto countIt = _pcFlagsCounters.begin();
    auto labelIt = Firmware::PcChangePacketFlagsLabels.begin();
    for(; (countIt != _pcFlagsCounters.end()) && (labelIt != Firmware::PcChangePacketFlagsLabels.end()); ++countIt, ++labelIt )
        std::cout << "  * " << std::left << std::setw(13) << *labelIt << " = " << *countIt << std::endl;
    std::cout << "- Last packet content: " << std::endl;
    printPCChangeLastPacketInfo();
    std::cout << "---------------------------------" << std::endl;
}

void MpsDb::printPCCounters() const
{

    std::cout << "Power Class Counters:"<< std::endl;
    std::cout << std::setw(124) << std::setfill('-') << "" << std::setfill(' ') << std::endl;

    // Print the header
    std::cout << std::setw(9) << "";
    for (std::size_t j {0}; j < (1<<POWER_CLASS_BIT_SIZE); ++j)
        std::cout << "pc[" << std::setw(2) << std::setfill('0') << j << "] " << std::setfill(' ');
    std::cout << std::endl;

    // Print the matrix
    for (std::size_t i {0}; i < NUM_DESTINATIONS; ++i) {
        std::cout << "dest[" << std::setw(2) << std::setfill('0') << i << "] " << std::setfill(' ');
        for (std::size_t j {0}; j < (1<<POWER_CLASS_BIT_SIZE); ++j) {
            std::cout << std::setw(6) << _pcCounters[i][j];
            std::cout << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::setw(124) << std::setfill('-') << "" << std::setfill(' ') << std::endl;
    std::cout << std::endl;

    printPCChangeInfo();
}

void MpsDb::clearUpdateTime() {
    _clearUpdateTime = true;
}

long MpsDb::getMaxUpdateTime() {
    return static_cast<int>( _inputUpdateTime.getAllMaxPeriod() * 1e6 );
}

long MpsDb::getAvgUpdateTime() {
    return static_cast<int>( _inputUpdateTime.getMeanPeriod() * 1e6 );
}

long MpsDb::getMaxFwUpdatePeriod() {
    return static_cast<int>( fwUpdateTimer.getAllMaxPeriod() * 1e6 );
}

long MpsDb::getAvgFwUpdatePeriod() {
    return static_cast<int>( fwUpdateTimer.getMeanPeriod() * 1e6 );
}

std::ostream & operator<<(std::ostream &os, MpsDb * const mpsDb)
{
    os << "Name: " << mpsDb->name << std::endl;

    mpsDb->printMap<DbCrateMapPtr, DbCrateMap::iterator>
        (os, mpsDb->crates, "Crate");

    mpsDb->printMap<DbApplicationTypeMapPtr, DbApplicationTypeMap::iterator>
        (os, mpsDb->applicationTypes, "ApplicationType");

    mpsDb->printMap<DbApplicationCardMapPtr, DbApplicationCardMap::iterator>
        (os, mpsDb->applicationCards, "ApplicationCard");

    mpsDb->printMap<DbBeamDestinationMapPtr, DbBeamDestinationMap::iterator>
        (os, mpsDb->beamDestinations, "BeamDestination");

    mpsDb->printMap<DbBeamClassMapPtr, DbBeamClassMap::iterator>
        (os, mpsDb->beamClasses, "BeamClass");

    mpsDb->printMap<DbFaultMapPtr, DbFaultMap::iterator>
        (os, mpsDb->faults, "Fault");

    mpsDb->printMap<DbFaultInputMapPtr, DbFaultInputMap::iterator>
        (os, mpsDb->faultInputs, "FaultInput");

    mpsDb->printMap<DbFaultStateMapPtr, DbFaultStateMap::iterator>
        (os, mpsDb->faultStates, "FaultState");

    mpsDb->printMap<DbIgnoreConditionMapPtr, DbIgnoreConditionMap::iterator>
        (os, mpsDb->ignoreConditions, "IgnoreCondition");

    mpsDb->printMap<DbAllowedClassMapPtr, DbAllowedClassMap::iterator>
        (os, mpsDb->allowedClasses, "AllowedClass (Mitigation)");

    mpsDb->printMap<DbDigitalChannelMapPtr, DbDigitalChannelMap::iterator>
        (os, mpsDb->digitalChannels, "DigitalChannel");

    mpsDb->printMap<DbAnalogChannelMapPtr, DbAnalogChannelMap::iterator>
        (os, mpsDb->analogChannels, "AnalogChannel");


    // TODO - Temporarily commented out, will delete once confirmed not needed
    // mpsDb->printMap<DbDeviceTypeMapPtr, DbDeviceTypeMap::iterator>
    //     (os, mpsDb->deviceTypes, "DeviceType");

    // mpsDb->printMap<DbDeviceStateMapPtr, DbDeviceStateMap::iterator>
    //     (os, mpsDb->deviceStates, "DeviceState");

    // mpsDb->printMap<DbDeviceInputMapPtr, DbDeviceInputMap::iterator>
    //     (os, mpsDb->deviceInputs, "DeviceInput");

    // TODO - Temporarily commented out, will delete once confirmed not needed
    // mpsDb->printMap<DbAllowedClassMapPtr, DbAllowedClassMap::iterator>
    //     (os, mpsDb->allowedClasses, "AllowedClass");




    return os;
}

std::vector<uint8_t> MpsDb::getFastUpdateBuffer()
{
    // Return a copy of the update buffer. We hold the mutex to avoid the buffer
    // to be re-write during the copy.
    std::lock_guard<std::mutex> lock(fwUpdateBufferMutex);
    return std::vector<uint8_t>( fwUpdateBuffer.begin(), fwUpdateBuffer.end() );
}

void MpsDb::fwUpdateReader()
{
    struct sched_param  param;
    param.sched_priority = 85;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
        perror("sched_setscheduler failed");

    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1)
        perror("mlockall failed");

    std::cout << "*** FW Update Data reader started" << std::endl;
    fwUpdateTimer.start();

    for(;;)
    {
        uint32_t received_size = 0;
        update_buffer_t buffer( fwUpdateBuferSize, 0 );
        while (received_size != buffer.size())
        {
            received_size = Firmware::getInstance().readUpdateStream(buffer.data(),
                buffer.size(), _inputUpdateTimeout);
#ifndef FW_ENABLED
            if (!run)
            {
                std::cout << "FW Update Data reader interrupted" << std::endl;
                return;
            }
#endif
            if (received_size == 0)
                ++_updateTimeoutCounter;
        }

        fwUpdateQueue.push(buffer);
        fwUpdateTimer.tick();
        fwUpdateTimer.start();
    }
}

void MpsDb::fwPCChangeReader()
{
    std::cout << "*** FW Power Class Change reader started" << std::endl;

    // 1k buffer, more than enough for "pc_change_t".
    uint8_t buffer[1024];

    while(run)
    {
        int64_t got {0};

        // Try to read, with a 10ms timeout, until we get a packet
        while(0 == got)
            got = Firmware::getInstance().readPCChangeStream(buffer, sizeof(buffer), 100000);

        if (got == sizeof(Firmware::pc_change_t)) {
            // Extract the message information
            Firmware::pc_change_t* pData { (Firmware::pc_change_t*)(buffer) };

            if (_pcChangeFirstPacket)
            {
                // On the first packet, we can not compare it with a previous one
                // so, we only count it as valid.
                ++_pcChangeCounter;
                _pcChangeFirstPacket = false;
            }
            else
            {
                // Calculate the difference between the tag number of the current
                // and previous packet. This difference will be:
                // == 1 : Ok, valid packet.
                // == 0 : packet with same seq. number
                // >  0 : lost packets (the difference less one is the number of missing packets)
                // <  0 : packet received out of order
                int64_t tagDelta { pData->tag - _pcChangeTag };

                if (1 == tagDelta)
                    ++_pcChangeCounter;
                else if (0 == tagDelta)
                    ++_pcChangeSameTagCounter;
                else if (tagDelta > 0)
                    _pcChangeLossCounter += (tagDelta - 1);
                else
                    ++_pcChangeOutOrderCounter;

            }

            // Save the message information
            _pcChangeTag = pData->tag;
            _pcChangeFlags = pData->flags;
            _pcChangeTimeStamp = pData->timeStamp;
            _pcChangePowerClass = pData->powerClass;

            // Apply the mask to the flags, to detect which error are set
            uint16_t errors { static_cast<uint16_t>(_pcChangeFlags ^ Firmware::PcChangePacketFlagsMask) };

            // Update the flag counters
            for (std::size_t i {0}; i < _pcFlagsCounters.size(); ++i)
            {
                if (errors & (1<<i))
                    ++_pcFlagsCounters[i];
            }

            // Valid packets doesn't have MonitorReady errors.
            // Count the number of packet that don't have MonitorReady errors, and increase the
            // corresponding power class transition counters.
            if (!(errors & 0x01))
            {
                uint64_t pcw { _pcChangePowerClass };
                for (std::size_t dest {0}; dest < NUM_DESTINATIONS; ++dest)
                {
                    uint8_t pc { static_cast<uint8_t>( pcw & ( (1<<POWER_CLASS_BIT_SIZE) - 1) ) };
                    ++_pcCounters[dest][pc];
                    pcw >>= POWER_CLASS_BIT_SIZE;
                }
            }

            // Print the received packet content if the debug flag is set
            if (_pcChangeDebug)
            {
                std::cout << "== Power Class Change Message ====" << std::endl;
                // Print packet bytes in hex
                for(std::size_t i {0}; i < sizeof(Firmware::pc_change_t); ++i)
                {
                    if (0 != i && 0 == i % 8)
                        std::cout << std::endl;
                    std::cout << std::setw(2) << std::setfill('0') << std::hex << unsigned(buffer[i]) << std::dec << " ";
                }
                std::cout << std::endl;

                // Print extracted info
                printPCChangeLastPacketInfo();
                std::cout << "==================================" << std::endl;
            }

        }
        else
        {
            // Increment bad size counter
            ++_pcChangeBadSizeCounter;
        }
    }

    std::cout << "FW Power Class Change reader interrupted" << std::endl;
}

void MpsDb::PCChangeSetDebug(bool debug)
{
    _pcChangeDebug = debug;
}

void MpsDb::mitigationWriter()
{
    struct sched_param  param;
    param.sched_priority = 87;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
        perror("sched_setscheduler failed");

    if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1)
        perror("mlockall failed");

    std::cout << "Mitigation writer started" << std::endl;

    for(;;)
    {
        // Pop a mitigation message from the queue, using the blocking call
        // (i.e. the thread will wait here until a value is available).
        mit_buffer_ptr_t p { softwareMitigationQueue.pop() };

        // Write the mitigation to FW
        mitigationTxTime.start();
        Firmware::getInstance().writeMitigation(*p);
        mitigationTxTime.tick();
    }
}

void MpsDb::inputProcessed()
{
    {
        std::lock_guard<std::mutex> lock(inputsUpdatedMutex);
        inputsUpdated = false;
    }
    inputsUpdatedCondVar.notify_one();
};

void MpsDb::pushMitBuffer()
{
    softwareMitigationQueue.push(softwareMitigationBuffer);
}

int MpsDb::getTotalDeviceCount()
{
    return digitalChannels->size() + analogChannels->size();
}

