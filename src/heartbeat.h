#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <functional>
#include <boost/atomic.hpp>
#include <cpsw_api_user.h>

#include "timer.h"

template <typename BeatPolicy>
class HeartBeat;

// Policy classes
class BlockingBeat;
class NonBlockingBeat;

// Convenience typdedef
typedef HeartBeat<BlockingBeat>    BlockingHeartBeat;
typedef HeartBeat<NonBlockingBeat> NonBlockingHeartBeat;

template <typename BeatPolicy>
class HeartBeat : public BeatPolicy
{
public:
    HeartBeat( Path root, const uint32_t& timeout=3500, size_t timerBufferSize=360 );
    ~HeartBeat();

    void setWdTime( const uint32_t& timeout );
    void printReport();

private:
    // FPGA clocks per microsecond. Used to convert the
    // maximum heartbeat period measured by the FW application.
    static const std::size_t fpgaClkPerUs = 250;

    ScalVal                 swWdTime;
    ScalVal_RO              swHbCntMax;
};


// Policy classes to define the type of heartbeat implementation

// Base policy class: contains all common functionality and interfaces
// to all the policy classes.
class BeatBase
{
public:
    BeatBase(Path root, size_t timerBufferSize);
    virtual ~BeatBase() {};

    virtual void beat() = 0;
    virtual void clear() = 0;
    virtual void printBeatReport() = 0;

    int  getWdErrorCnt() const       { return wdErrorCnt; };

    const double getMinTxPeriod()    { return txPeriod.getMinPeriod();  };
    const double getMaxTxPeriod()    { return txPeriod.getAllMaxPeriod();  };
    const double getMeanTxPeriod()   { return txPeriod.getMeanPeriod(); };

    const double getMinTxDuration()  { return txDuration.getMeanPeriod(); };
    const double getMaxTxDuration()  { return txDuration.getAllMaxPeriod();  };
    const double getMeanTxDuration() { return txDuration.getMeanPeriod(); };

protected:
    // Some default implementation which could be used by several derived classes
    void defaultBeat();
    void defaultClear();
    void defaultPrintReport();

private:
    Timer<double> txPeriod;
    Timer<double> txDuration;
    ScalVal_RO    swWdError;
    Command       swHeartBeat;
    int           hbCnt;
    int           wdErrorCnt;
};

// Blocking beat policy class: this class send heratbeat in the foreground, so the
// call to the 'beat' method will block and return after the heartbeat command is
// sent to the FW.
class BlockingBeat : public BeatBase
{
public:
    BlockingBeat(Path root, size_t timerBufferSize);
    virtual ~BlockingBeat() {};

    virtual void beat();
    virtual void clear();
    virtual void printBeatReport();
};

// Non blocking beat policy class: this class send heartbeat on a background thread,
// so the call to the 'beat' method will return before the hearbeat command is sent
// to the FW.
class NonBlockingBeat : public BeatBase
{
public:
    NonBlockingBeat(Path root, size_t timerBufferSize);
    virtual ~NonBlockingBeat();

    virtual void beat();
    virtual void clear();
    virtual void printBeatReport();

private:
    std::size_t             reqTimeoutCnt;
    std::size_t             reqTimeout;
    bool                    beatReq;
    std::mutex              beatMutex;
    std::condition_variable beatCondVar;
    boost::atomic<bool>     run;
    std::thread             beatThread;

    void beatWriter();

    // Helper predicate functions used for the conditional variable
    // 'wait_for' methods. We need this as our old rhel6 host don't
    // support C++11 , otherwise we could have used lambda expressions.
    bool pred()  { return beatReq;  };
    bool predN() { return !beatReq; };
};

#endif
