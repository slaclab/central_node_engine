#include "heartbeat.h"

#ifdef FW_ENABLED
HeartBeat::HeartBeat( Path root, const uint32_t& timeout, size_t timerBufferSize )
:
    txPeriod     ( "Time Between Heartbeats", timerBufferSize ),
    txDuration   ( "Time to send Heartbeats", timerBufferSize ),
    swWdTime     ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdTime" ) ) ),
    swWdError    ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdError" ) ) ),
    swHeartBeat  ( ICommand::create     ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SwHeartbeat" ) ) ),
    hbCnt        ( 0 ),
    wdErrorCnt   ( 0 ),
    beatReq      ( false ),
    run          ( true ),
    beatThread   ( std::thread( &HeartBeat::beatWriter, this ) )
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
}

void HeartBeat::printReport()
{
    printf( "\n" );
    printf( "HeartBeat report:\n" );
    printf( "===============================================\n" );
    uint32_t u32;
    swWdTime->getVal( &u32 );
    printf( "Software watchdog timer:           %" PRIu32 " us\n", u32 );
    printf( "Heartbeat count:                   %d\n",    hbCnt );
    printf( "Software watchdog error count:     %d\n",    wdErrorCnt );
    printf( "Maximum period between heartbeats: %f us\n", ( txPeriod.getMaxPeriod()    * 1000000 ) );
    printf( "Average period between heartbeats: %f us\n", ( txPeriod.getMeanPeriod()   * 1000000 ) );
    printf( "Minimum period between heartbeats: %f us\n", ( txPeriod.getMinPeriod()    * 1000000 ) );
    printf( "Maximum period to send heartbeats: %f us\n", ( txDuration.getMaxPeriod()  * 1000000 ) );
    printf( "Average period to send heartbeats: %f us\n", ( txDuration.getMeanPeriod() * 1000000 ) );
    printf( "Minimum period to send heartbeats: %f us\n", ( txDuration.getMinPeriod()  * 1000000 ) );
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
    beatCondVar.notify_all();
}

void HeartBeat::beatWriter()
{
    std::cout << "Heartbeat writer thread started..." << std::endl;

    // Declare as real time task
    struct sched_param  param;
    param.sched_priority = 20;
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1)
    {
        perror("Set priority");
        std::cerr << "WARN: Setting thread RT priority failed on Heartbeat thread." << std::endl;
    }

    for(;;)
    {
        {
            // Wait for a request
            std::unique_lock<std::mutex> lock(beatMutex);
            while(!beatReq)
            {
                beatCondVar.wait_for( lock, std::chrono::milliseconds(5) );
                if(!run)
                {
                    std::cout << "Heartbeat writer thread interrupted" << std::endl;
                    return;
                }
            }
        }

        // Start the TX duration Timer
        txDuration.start();

        // Check if there was a WD error, and increase counter accordingly
        uint32_t u32;
        swWdError->getVal( &u32 );
        if ( u32 )
            ++wdErrorCnt;

        // Set heartbeat bit
        swHeartBeat->execute();

        // Tick the duration timer
        txDuration.tick();

        // Tick period timer;
        txPeriod.tick();

        // Increase counter
        ++hbCnt;

        beatReq = false;
    }
}
#endif
