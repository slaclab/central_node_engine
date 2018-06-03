#include "heartbeat.h"

HeartBeat::HeartBeat( Path root, const uint32_t& timeout )
:
    txPeriod       ( "Time Between Heartbeats", 720 ),
    txDuration     ( "Time to send Heartbeats", 720 ),
    swWdTime       ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdTime" ) ) ),
    swWdError      ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdError" ) ) ),
    swHeartBeat2   ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdHeartbeat" ) ) ),
    swHeartBeat    ( ICommand::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SwHeartbeat" ) ) ),
    hbCnt          ( 0 ),
    wdErrorCnt     ( 0 ),
    beatReq        ( false )
{
    printf("\n");
    printf("Central Node HeartBeat started.\n");
    swWdTime->setVal( timeout );
    printf("Software Watchdog timer set to: %" PRIu32 "\n", timeout);

    // Start heartbeat thread
    beatThread = boost::thread( &HeartBeat::beatWriter, this );
    if( pthread_setname_np( beatThread.native_handle(), "HeartBeat" ) )
        perror( "pthread_setname_np failed for HeartBeat" );


}

HeartBeat::~HeartBeat()
{
    // Stop the heartbeat thread
    beatThread.interrupt();
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
    printf( "Software waychdog timer:           %" PRIu32 " us\n", u32 );
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

    // Start the period timer
    txPeriod.start();

    try
    {
        for(;;)
        {
            // Wait for a request
            {
                std::unique_lock<std::mutex> lock(beatMutex);
                while(!beatReq)
                {
                    beatCondVar.wait(lock);
                }
            }

            // Start the TX duration Timer
            txDuration.start();

            // Check if there was a WD error, and increase counter accordingly
            uint32_t u32;
            swWdError->getVal( &u32 );
            if ( u32 )
                ++wdErrorCnt;

            // Set heartbeat command
            // swHeartBeat->execute();
            swHeartBeat2->setVal( static_cast<uint64_t>( 0 ) );

            // Tick period timer;
            txPeriod.tick();

            // Increase counter
            ++hbCnt;

            // Tick the duration timer
            txDuration.tick();

            swHeartBeat2->setVal( static_cast<uint64_t>( 1 ) );

            beatReq = false;
        }
    }
    catch(boost::thread_interrupted& e)
    {
        std::cout << "Heartbeat writer thread interrupted" << std::endl;
    }
}