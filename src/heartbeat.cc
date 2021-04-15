#include "heartbeat.h"

#ifdef FW_ENABLED
HeartBeat::HeartBeat( Path root, const uint32_t& timeout, size_t timerBufferSize )
:
    txPeriod      ( "Time Between Heartbeats", timerBufferSize ),
    txDuration    ( "Time to send Heartbeats", timerBufferSize ),
    swWdTime      ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdTime" ) ) ),
    swWdError     ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdError" ) ) ),
    swHbCntMax    ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareHbUpCntMax" ) ) ),
    swHeartBeat   ( ICommand::create     ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SwHeartbeat" ) ) ),
    hbCnt         ( 0 ),
    wdErrorCnt    ( 0 ),
    reqTimeoutCnt ( 0 ),
    reqTimeout    ( 5 ),
    beatReq       ( false ),
    run           ( true ),
    beatThread    ( std::thread( &HeartBeat::beatWriter, this ) )
{
    printf("\n");
    printf("Central Node HeartBeat started.\n");
    swWdTime->setVal( timeout );
    printf("Software Watchdog timer set to: %" PRIu32 "\n", timeout);

    if( pthread_setname_np( beatThread.native_handle(), "HeartBeat" ) )
        perror( "pthread_setname_np failed for HeartBeat thread" );
}

HeartBeat::~HeartBeat()
{
    // Stop the heartbeat thread
    run = false;
    beatThread.join();

    // Print final report
    printReport();
    std::cout << std::flush;
}

void HeartBeat::clear()
{
    // Wait for any heartbeat generation in progress
    std::unique_lock<std::mutex> lock(beatMutex);
    beatCondVar.wait( lock, std::bind(&HeartBeat::predN, this) );

    txPeriod.clear();
    txDuration.clear();
    hbCnt = 0;
    wdErrorCnt = 0;
    reqTimeoutCnt = 0;
}

void HeartBeat::printReport()
{
    printf( "\n" );
    printf( "HeartBeat report:\n" );
    printf( "===============================================\n" );
    uint32_t u32;
    swWdTime->getVal( &u32 );
    printf( "Software watchdog timer       : %" PRIu32 " us\n", u32 );
    printf( "Request timeout               : %zu ms\n", reqTimeout );
    printf( "Heartbeat count               : %d\n",    hbCnt );
    printf( "Software watchdog error count : %d\n",    wdErrorCnt );
    printf( "Timeouts waiting for requests : %zu\n",   reqTimeoutCnt );
    if ( 0 != hbCnt)
    {
        // Print the maximum period measured by the FW application
        uint32_t u32;
        swHbCntMax->getVal(&u32);
        printf( "Maximum period between heartbeats (FW)  : %zu us\n", u32/fpgaClkPerUs );

        printf( "Maximum period between heartbeats (All) : %f us\n", ( txPeriod.getAllMaxPeriod()   * 1000000 ) );
        printf( "Maximum period between heartbeats       : %f us\n", ( txPeriod.getMaxPeriod()      * 1000000 ) );
        printf( "Average period between heartbeats       : %f us\n", ( txPeriod.getMeanPeriod()     * 1000000 ) );
        printf( "Minimum period between heartbeats       : %f us\n", ( txPeriod.getMinPeriod()      * 1000000 ) );
        printf( "Maximum period to send heartbeats (All) : %f us\n", ( txDuration.getAllMaxPeriod() * 1000000 ) );
        printf( "Maximum period to send heartbeats       : %f us\n", ( txDuration.getMaxPeriod()    * 1000000 ) );
        printf( "Average period to send heartbeats       : %f us\n", ( txDuration.getMeanPeriod()   * 1000000 ) );
        printf( "Minimum period to send heartbeats       : %f us\n", ( txDuration.getMinPeriod()    * 1000000 ) );
    }
    printf( "===============================================\n" );
    printf( "\n" );
}

void HeartBeat::setWdTime( const uint32_t& timeout )
{
    swWdTime->setVal( timeout );
}

void HeartBeat::beat()
{
    std::unique_lock<std::mutex> lock(beatMutex);
    beatReq = true;
    beatCondVar.notify_one();
}

void HeartBeat::beatWriter()
{
    std::cout << "Heartbeat writer thread started..." << std::endl;

    // Declare as real time task
    struct sched_param  param;
    param.sched_priority = 74;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("Set priority");
        std::cerr << "WARN: Setting thread RT priority failed on Heartbeat thread." << std::endl;
    }

    for(;;)
    {
        // Wait for a request
        std::unique_lock<std::mutex> lock(beatMutex);
        if (beatCondVar.wait_for(lock, std::chrono::milliseconds(reqTimeout), std::bind(&HeartBeat::pred, this)))
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

            // Notify that we are done with the heartbeat generation.
            // Manual unlocking is done before notifying, to avoid waking up
            // the waiting thread only to block again (see notify_one for details)
            beatReq = false;
            lock.unlock();
            beatCondVar.notify_one();
        }
        else
        {
            if (run)
                ++reqTimeoutCnt;
        }

        if (!run)
        {
            std::cout << "Heartbeat writer thread interrupted" << std::endl;
            return;
        }
    }
}
#endif
