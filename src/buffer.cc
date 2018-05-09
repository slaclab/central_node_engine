#include "buffer.h"

DataBuffer::DataBuffer( const size_t& size )
:
mySize( size ),
buf0( size ),
buf1( size ),
writePtr( &buf0 ),
readPtr( &buf1 ),
writeDone( false ),
readDone( true ),
writeCnt( 0 ),
readCnt( 0 ),
ready( false )
{
    printf( "Circular buffer was created (size = %zu)\n", mySize );
}

void DataBuffer::doneWriting()
{
    std::lock_guard<std::mutex> lock(mut);
    ++writeCnt;
    writeDone = true;
    tryToRotate();
}

void DataBuffer::doneReading()
{
    std::lock_guard<std::mutex> lock(mut);
    ++readCnt;
    readDone = true;
    tryToRotate();
}

void DataBuffer::tryToRotate()
{
    if ( writeDone and readDone )
    {
        std::swap(writePtr, readPtr);
        writeDone = false;
        readDone  = false;
        ready     = true;
        condVar.notify_all();
    }
    else
    {
        ready = false;
    }
}

DataBuffer::~DataBuffer()
{
    printf( "\n" );
    printf( "DataBuffer report:\n" );
    printf( "==========================================\n" );
    printf( "Number of write operations: %d\n",  writeCnt );
    printf( "Number of read operations:  %d\n",  readCnt );
    printf( "Buffer[0] size:             %zu\n", buf0.size() );
    printf( "Buffer[1] size:             %zu\n", buf1.size() );
    printf( "==========================================\n" );
    printf( "\n" );
}
