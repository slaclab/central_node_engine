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

    MpsDbPtr getCurrentDb();
    BypassManagerPtr getBypassManager();

    // Watchdog error counter
    int getWdErrorCnt() const { return hb.getWdErrorCnt(); };

    // Watchdog times
    long getAvgWdUpdatePeriod();
    long getMaxWdUpdatePeriod();

private:
    void mitigate();

    void setTentativeBeamClass();
    void setAllowedBeamClass();

    void evaluateFaults();
    bool evaluateIgnoreConditions();
    void mitgate();

    MpsDbPtr _mpsDb;
    BypassManagerPtr _bypassManager;

    DbBeamClassPtr _highestBeamClass;
    DbBeamClassPtr _lowestBeamClass;

    std::stringstream _errorStream;
    Timer<double> _checkFaultTime;

    Timer<double>  _evaluationCycleTime;

    // Heartbeat control class
    HeartBeat hb;

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
