#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <boost/atomic.hpp>
#include <cpsw_api_user.h>

#include "timer.h"

class HeartBeat
{
public:
    HeartBeat( Path root, const uint32_t& timeout=3500, size_t timerBufferSize=360 );
    ~HeartBeat();
    void beat();
    void setWdTime( const uint32_t& timeout );

    void printReport();
    void clear();
    int  getWdErrorCnt() const       { return wdErrorCnt; };

    const double getMinTxPeriod()    { return txPeriod.getMinPeriod();  };
    const double getMaxTxPeriod()    { return txPeriod.getAllMaxPeriod();  };
    const double getMeanTxPeriod()   { return txPeriod.getMeanPeriod(); };

    const double getMinTxDuration()  { return txDuration.getMeanPeriod(); };
    const double getMaxTxDuration()  { return txDuration.getAllMaxPeriod();  };
    const double getMeanTxDuration() { return txDuration.getMeanPeriod(); };

private:
    // FPGA clocks per microsecond. Used to convert the
    // maximum heartbeat period measured by the FW application.
    static const std::size_t fpgaClkPerUs = 250;

    Timer<double>           txPeriod;
    Timer<double>           txDuration;
    ScalVal                 swWdTime;
    ScalVal_RO              swWdError;
    ScalVal_RO              swHbCntMax;
    Command                 swHeartBeat;
    int                     hbCnt;
    int                     wdErrorCnt;
    std::size_t             reqTimeoutCnt;
    std::size_t             reqTimeout;
    bool                    beatReq;
    std::mutex              beatMutex;
    std::condition_variable beatCondVar;
    boost::atomic<bool>     run;
    std::thread             beatThread;

    void        beatWriter();
};

#endif
