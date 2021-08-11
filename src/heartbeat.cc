#include "heartbeat.h"

#ifdef FW_ENABLED
template <typename BeatPolicy>
HeartBeat<BeatPolicy>::HeartBeat( Path root, const uint32_t& timeout, size_t timerBufferSize )
:
    BeatPolicy    ( root, timerBufferSize ),
    swWdTime      ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdTime" ) ) ),
    swHbCntMax    ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareHbUpCntMax" ) ) )
{
    printf("\n");
    printf("Central Node HeartBeat started.\n");
    swWdTime->setVal( timeout );
    printf("Software Watchdog timer set to: %" PRIu32 "\n", timeout);
}

template<typename BeatPolicy>
HeartBeat<BeatPolicy>::~HeartBeat()
{
    // Print final report
    printReport();
    std::cout << std::flush;
}

template<typename BeatPolicy>
void HeartBeat<BeatPolicy>::printReport()
{
    printf( "\n" );
    printf( "HeartBeat report:\n" );
    printf( "===============================================\n" );
    uint32_t u32;
    swWdTime->getVal( &u32 );
    printf( "Software watchdog timer       : %" PRIu32 " us\n", u32 );
    swHbCntMax->getVal(&u32);
    printf( "Maximum heartbeat period (FW) : %zu us\n", u32/fpgaClkPerUs );
    this->printBeatReport();
}

template<typename BeatPolicy>
void HeartBeat<BeatPolicy>::setWdTime( const uint32_t& timeout )
{
    swWdTime->setVal( timeout );
}

////////////////////////////////
// BeatBase class definitions //
////////////////////////////////
BeatBase::BeatBase(Path root, size_t timerBufferSize)
:
    txPeriod      ( "Time Between Heartbeats", timerBufferSize ),
    txDuration    ( "Time to send Heartbeats", timerBufferSize ),
    swWdError     ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdError" ) ) ),
    swHeartBeat   ( ICommand::create     ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SwHeartbeat" ) ) ),
    hbCnt         ( 0 ),
    wdErrorCnt    ( 0 )
{
}

void BeatBase::defaultClear()
{
    txPeriod.clear();
    txDuration.clear();
    hbCnt = 0;
    wdErrorCnt = 0;
}

void BeatBase::defaultBeat()
{
    // Start the TX duration timer
    txDuration.start();

    // Check if there was a WD error, and increase counter accordingly
    uint32_t u32;
    swWdError->getVal(&u32);
    if (u32)
        ++wdErrorCnt;

    // Set heartbeat bit
    swHeartBeat->execute();

    // Tick period timer;
    txPeriod.tick();

    // Increase counter
    ++hbCnt;

    // Tick the TX duration timer
    txDuration.tick();
}

void BeatBase::defaultPrintReport()
{
    printf( "Heartbeat count               : %d\n",    hbCnt );
    printf( "Software watchdog error count : %d\n",    wdErrorCnt );
    txPeriod.show();
    txDuration.show();
}

////////////////////////////////////
// BlockingBeat class definitions //
////////////////////////////////////
BlockingBeat::BlockingBeat(Path root, size_t timerBufferSize)
:
    BeatBase ( root, timerBufferSize )
{
}

void BlockingBeat::clear()
{
    defaultClear();
}

void BlockingBeat::beat()
{
    defaultBeat();
}

void BlockingBeat::printBeatReport()
{
    defaultPrintReport();
}

///////////////////////////////////////
// NonBlockingBeat class definitions //
///////////////////////////////////////
NonBlockingBeat::NonBlockingBeat(Path root, size_t timerBufferSize)
:
    BeatBase        ( root, timerBufferSize ),
    reqTimeoutCnt   ( 0 ),
    reqTimeout      ( 5 ),
    beatReqCount    ( 0 ),
    beatReqCountMax ( 0 ),
    run             ( true ),
    beatThread      ( std::thread( &NonBlockingBeat::beatWriter, this ) )
{
    if( pthread_setname_np( beatThread.native_handle(), "HeartBeat" ) )
       perror( "pthread_setname_np failed for HeartBeat thread" );
}

NonBlockingBeat::~NonBlockingBeat()
{
    // Stop the heartbeat thread
    run = false;
    beatThread.join();
}

void NonBlockingBeat::clear()
{
    {
        std::lock_guard<std::mutex> lock(beatMutex);
        beatReqCountMax = 0;
        reqTimeoutCnt = 0;
    }
    defaultClear();
}

void NonBlockingBeat::beat()
{
    {
        std::lock_guard<std::mutex> lock(beatMutex);
        // Increase the request counter
        ++beatReqCount;

        // Keep track of the maximum number of requests without handling
        if (beatReqCount > beatReqCountMax)
            beatReqCountMax = beatReqCount;
    }

    beatCondVar.notify_one();
}

void NonBlockingBeat::beatWriter()
{
    std::cout << "Heartbeat writer thread started..." << std::endl;

    // Declare as real time task
    struct sched_param  param;
    param.sched_priority = 87;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("Set priority");
        std::cerr << "WARN: Setting thread RT priority failed on Heartbeat thread." << std::endl;
    }

    for(;;)
    {
        // Wait for a request
        std::unique_lock<std::mutex> lock(beatMutex);
        if (beatCondVar.wait_for(lock, std::chrono::milliseconds(reqTimeout), std::bind(&NonBlockingBeat::pred, this)))
        {
            --beatReqCount;
            lock.unlock();

            defaultBeat();
        }
        else
        {
            ++reqTimeoutCnt;
        }

        if (!run)
        {
            std::cout << "Heartbeat writer thread interrupted" << std::endl;
            return;
        }
    }
}

void NonBlockingBeat::printBeatReport()
{
    std::size_t max, cnt;

    {
        std::lock_guard<std::mutex> lock(beatMutex);
        max = beatReqCountMax;
        cnt = reqTimeoutCnt;
    }

    printf( "Request timeout               : %zu ms\n", reqTimeout );
    printf( "Timeouts waiting for requests : %zu\n",    cnt        );
    printf( "Maximum queued requests       : %zu\n",    max        );
    defaultPrintReport();
}

// Explicit template instantiations
template class HeartBeat<BlockingBeat>;
template class HeartBeat<NonBlockingBeat>;

#endif
