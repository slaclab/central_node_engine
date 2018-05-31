#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <boost/thread/thread.hpp>
#include <mutex>
#include <condition_variable>
#include <cpsw_api_user.h>

#include "timer.h"

class HeartBeat
{
public:
    HeartBeat( Path root, const uint32_t& timeout=3500 );
    ~HeartBeat();
    void beat();
    void setWdTime( const uint32_t& timeout );

    void printReport();
    int  getWdErrorCnt() const { return wdErrorCnt; };

private:
    Timer<double>           txPeriod;
    Timer<double>           txDuration;
    ScalVal                 swWdTime;
    ScalVal_RO              swWdError;
    ScalVal                 swHeartBeat2;
    Command                 swHeartBeat;
    int                     hbCnt;
    int                     wdErrorCnt;
    boost::thread           beatThread;
    bool                    beatReq;
    std::mutex              beatMutex;
    std::condition_variable beatCondVar;

    void beatWriter();
};

#endif