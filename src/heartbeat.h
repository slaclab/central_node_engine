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
    int  getWdErrorCnt() const       { return wdErrorCnt; };

    const double getMinTxPeriod()    { return txPeriod.getMinPeriod();  };
    const double getMaxTxPeriod()    { return txPeriod.getMaxPeriod();  };
    const double getMeanTxPeriod()   { return txPeriod.getMeanPeriod(); };

    const double getMinTxDuration()  { return txDuration.getMeanPeriod(); };
    const double getMaxTxDuration()  { return txDuration.getMaxPeriod();  };
    const double getMeanTxDuration() { return txDuration.getMeanPeriod(); };

private:
    Timer<double>           txPeriod;
    Timer<double>           txDuration;
    ScalVal                 swWdTime;
    ScalVal_RO              swWdError;
    ScalVal                 swHeartBeat2;
    Command                 swHeartBeat;
    int                     hbCnt;
    int                     wdErrorCnt;
    bool                    beatReq;
    std::mutex              beatMutex;
    std::condition_variable beatCondVar;
    boost::atomic<bool>     run;
    std::thread             beatThread;
    
    void        beatWriter();
};

#endif
