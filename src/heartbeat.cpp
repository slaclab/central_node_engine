#include "heartbeat.h"

HeartBeat::HeartBeat( Path root, const uint32_t& timeout )
:
    txPeriod       ( "Time Between Heartbeats", 360 ),
    txDuration     ( "Time to send Heartbeats", 360 ),
    swWdTime       ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdTime" ) ) ),
    swWdError      ( IScalVal_RO::create ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdError" ) ) ),
    swHeartBeat2   ( IScalVal::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SoftwareWdHeartbeat" ) ) ),
    swHeartBeat    ( ICommand::create    ( root->findByName( "/mmio/MpsCentralApplication/MpsCentralNodeCore/SwHeartbeat" ) ) ),
    hbCnt          ( 0 ),
    wdErrorCnt     ( 0 )
{
    printf("\n");
    printf("Central Node HeartBeat started.\n");
    swWdTime->setVal( timeout );
    printf("Software Watchdog timer set to: %" PRIu32 "\n", timeout);

    // Start the period timer
    txPeriod.start();
}

HeartBeat::~HeartBeat()
{
    printReport();
}

void HeartBeat::printReport()
{
    printf( "\n" );
    printf( "HeartBeat report:\n" );
    printf( "===============================================\n" );
    printf( "Stopping HeartBeat object:\n" );
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
        // Start the TX duration Timer
        txDuration.start();

        // Check if there was a WD error, and increase counter accordingly
        uint32_t u32;
        swWdError->getVal( &u32 );
        if ( u32 )
            ++wdErrorCnt;

        // Set heartbeat command
        swHeartBeat->execute();

        // Tick period timer;
        txPeriod.tick();

        // Increase counter
        ++hbCnt;

        // Tick the duration timer
        txDuration.tick();
}