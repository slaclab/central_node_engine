#ifndef _HEARTBEAT_H_
#define _HEARTBEAT_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <cpsw_api_user.h>

#include "timer.h"

class HeartBeat
{
public:
    HeartBeat( Path root, const uint32_t& timeout=3500 );
    ~HeartBeat();

    void setWdTime( const uint32_t& timeout );
    void beat();
    void printReport();
    int  getWdErrorCnt() const { return wdErrorCnt; };

private:
    Timer<double>   txPeriod;
    Timer<double>   txDuration;
    ScalVal         swWdTime;
    ScalVal_RO      swWdError;
    ScalVal         swHeartBeat2;
    Command         swHeartBeat;
    int             hbCnt;
    int             wdErrorCnt;
};

#endif