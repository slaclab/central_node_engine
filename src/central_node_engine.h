#ifndef CENTRAL_NODE_ENGINE_H
#define CENTRAL_NODE_ENGINE_H

#include <iostream>
#include <sstream>
#include <thread>
#include <boost/shared_ptr.hpp>

#include <central_node_exception.h>
#include <central_node_yaml.h>
#include <central_node_database.h>
#include <central_node_bypass.h>
#include <central_node_bypass_manager.h>
#include <time_util.h>
#include "timer.h"
#include "buffer.h"
#include "heartbeat.h"

class EngineException: public CentralNodeException
{
public:
    explicit EngineException(const char* message) : CentralNodeException(message) {}
    explicit EngineException(const std::string& message) : CentralNodeException(message) {}
    virtual ~EngineException() throw () {}
};

/**
 * Evaluation Cycle:
 * - Read new info from firmware (inputs and mitigation info)
 * - Update values for digital/analog inputs
 * - Update values for digital/analog faults
 * - Check allowed beam at mitigation devices
 * - Request mitigation
 */

class Engine
{
private:
    Engine();
    Engine(Engine const &);
    ~Engine();

    void operator=(Engine const &);
    bool _initialized;
    std::thread *_engineThread;

    static std::mutex _mutex;
    static volatile bool _evaluate;
    static uint32_t _rate;
    static uint32_t _updateCounter;
    static time_t _startTime;
    static bool _enableMps;

    uint32_t _debugCounter;
    static uint32_t _inputUpdateFailCounter;

public:
    int loadConfig(std::string yamlFileName, uint32_t inputUpdateTimeout=3500);
    int reloadConfig();
    int reloadConfigFromIgnore();
    int checkFaults();
    bool isInitialized();
    void clearSoftwareLatch();

    void threadExit();
    void threadJoin();

    friend class EngineTest;
    friend class BypassTest;
    friend class FirmwareTest;

    void showFaults();
    void showStats();
    void showBeamDestinations();
    void showDeviceInputs();
    void showFirmware();
    void showDatabaseInfo();

    uint32_t getUpdateRate();
    uint32_t getUpdateCounter();
    time_t getStartTime();

    long getMaxCheckTime();
    long getAvgCheckTime();
    long getMaxEvalTime();
    long getAvgEvalTime();
    void clearMaxTimers();
    bool unlatchAllowed();
    void startLatchTimeout();

    MpsDbPtr getCurrentDb();
    BypassManagerPtr getBypassManager();

    // Watchdog error counter
    int getWdErrorCnt() const { return hb.getWdErrorCnt(); };

    // Watchdog times
    long getAvgWdUpdatePeriod();
    long getMaxWdUpdatePeriod();

private:
    void mitigate();

    bool findShutterDevice();
    bool findBeamDestinations();

    void setTentativeBeamClass();
    bool setAllowedBeamClass();

    void evaluateFaults();
    bool evaluateIgnoreConditions();
    void setFaultIgnore();
    void breakAnalogIgnore();

    MpsDbPtr _mpsDb;
    BypassManagerPtr _bypassManager;

    DbBeamClassPtr _highestBeamClass;
    DbBeamClassPtr _lowestBeamClass;

    DbDigitalDevicePtr _shutterDevice; // Needed to monitor shutter closed status
    uint32_t _shutterClosedStatus; // Current shutter closed status
    bool _aomAllowWhileShutterClosed; // When true enable AOM while shutter is closed
    uint32_t _aomAllowEnableCounter; // Increase every time AOM is enabled while shutter is closed
    uint32_t _aomAllowDisableCounter; // Decrease every time AOM is enabled while shutter is closed
    bool _linacFwLatch; // Latches in SW if the FW is latched during config reloads
    uint32_t _reloadCount; // Counts the number of FW config reloads (after ignore and AOM enable)
    bool _unlatchAllowed;

    DbBeamDestinationPtr _linacDestination; // Used to directly set PC by the engine
    DbBeamDestinationPtr _aomDestination; // Used to directly set PC by the engine

    std::stringstream _errorStream;
    Timer<double> _checkFaultTime;
    Timer<double> _unlatchTimer;
    Timer<double>  _evaluationCycleTime;

    // Heartbeat control class
    NonBlockingHeartBeat hb;

public:
    static Engine &getInstance()
    {
        static Engine instance;
        return instance;
    }

    void startUpdateThread();
    void engineThread();

    friend class MpsDb;
};

typedef boost::shared_ptr<Engine> EnginePtr;

#endif
