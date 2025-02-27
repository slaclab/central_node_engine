#include "heartbeat.h"

#ifndef FW_ENABLED

#warning "Code compiled without CPSW - fake heartbeat core"
template <typename BeatPolicy>
HeartBeat<BeatPolicy>::HeartBeat( Path root, const uint32_t& timeout, size_t timerBufferSize )
:
    txPeriod     ( "Time Between Heartbeats", timerBufferSize ),
    txDuration   ( "Time to send Heartbeats", timerBufferSize ),
    hbCnt        ( 0 ),
    wdErrorCnt   ( 0 ),
    beatReq      ( false ),
    run          ( true ),
    beatThread   ( std::thread( &HeartBeat::beatWriter, this ) )
{
    printf("\n");
    printf("Central Node HeartBeat started.\n");

    if( pthread_setname_np( beatThread.native_handle(), "HeartBeat" ) )
        perror( "pthread_setname_np failed for HeartBeat thread" );
}
template<typename BeatPolicy>
HeartBeat<BeatPolicy>::~HeartBeat()
{
    // Stop the heartbeat thread
    run = false;
    beatThread.join();

    // Print final report
    printReport();
}
template<typename BeatPolicy>
void HeartBeat<BeatPolicy>::printReport()
{
    printf( "\n" );
    printf( "HeartBeat report:\n" );
    printf( "===============================================\n" );
    printf( "Heartbeat count:                   %d\n",    hbCnt );
    printf( "Software watchdog error count:     %d\n",    wdErrorCnt );
    printf( "Maximum period between heartbeats: %f us\n", ( txPeriod.getAllMaxPeriod()    * 1000000 ) );
    printf( "Average period between heartbeats: %f us\n", ( txPeriod.getMeanPeriod()   * 1000000 ) );
    printf( "Minimum period between heartbeats: %f us\n", ( txPeriod.getMinPeriod()    * 1000000 ) );
    printf( "Maximum period to send heartbeats: %f us\n", ( txDuration.getAllMaxPeriod()  * 1000000 ) );
    printf( "Average period to send heartbeats: %f us\n", ( txDuration.getMeanPeriod() * 1000000 ) );
    printf( "Minimum period to send heartbeats: %f us\n", ( txDuration.getMinPeriod()  * 1000000 ) );
    printf( "===============================================\n" );
    printf( "\n" );
}
template<typename BeatPolicy>
void HeartBeat<BeatPolicy>::setWdTime( const uint32_t& timeout )
{
}
template<typename BeatPolicy>
void HeartBeat<BeatPolicy>::beat()
{
    std::unique_lock<std::mutex> lock(beatMutex);
    beatReq = true;
    beatCondVar.notify_all();
}
template<typename BeatPolicy>
void HeartBeat<BeatPolicy>::beatWriter()
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

    // Start the period timer
    txPeriod.start();

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

        // Set heartbeat command

        // Tick period timer;
        txPeriod.tick();

        // Increase counter
        ++hbCnt;

        // Tick the duration timer
        txDuration.tick();

        beatReq = false;
    }
}
#endif
