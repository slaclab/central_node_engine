#include "buffer.h"

template <typename T>
DataBuffer<T>::DataBuffer( const size_t& size )
:
mySize( size ),
buf0( size ),
buf1( size ),
writePtr( &buf0 ),
readPtr( &buf1 ),
writeDone( false ),
readDone( true ),
writeCnt( 0 ),
readCnt( 0 )
{
    printf( "Circular buffer was created (size = %zu)\n", mySize );
}

template <typename T>
void DataBuffer<T>::doneWriting()
{
    std::lock_guard<std::mutex> lock(mut);
    ++writeCnt;
    writeDone = true;
    tryToRotate();
}

template <typename T>
void DataBuffer<T>::doneReading()
{
    std::lock_guard<std::mutex> lock(mut);
    ++readCnt;
    readDone = true;
    tryToRotate();
}

template <typename T>
void DataBuffer<T>::tryToRotate()
{
    if ( writeDone and readDone )
    {
        std::swap(writePtr, readPtr);
        writeDone = false;
        readDone  = false;
        condVar.notify_all();
    }
}

template <typename T>
DataBuffer<T>::~DataBuffer()
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

template class DataBuffer<uint8_t>;