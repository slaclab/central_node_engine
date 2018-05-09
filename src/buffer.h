#ifndef _BUFFER_H_
#define _BUFFER_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <vector>
#include <mutex>
#include <condition_variable>

class DataBuffer
{
public:
    DataBuffer( const size_t& size );
    ~DataBuffer();

    std::vector<uint8_t>* getWritePtr()   const { return writePtr;    };
    std::vector<uint8_t>* getReadPtr()    const { return readPtr;     };
    size_t   size()                       const { return mySize;      };
    void     doneWriting();
    void     doneReading();
    bool                     isReady()    const { return ready;       };
    std::condition_variable* getCondVar()       { return &condVar;    };
    std::mutex*              getMutex()         { return &mut;        };

private:
    size_t                  mySize;
    std::vector<uint8_t>    buf0;
    std::vector<uint8_t>    buf1;
    std::vector<uint8_t>*   writePtr;
    std::vector<uint8_t>*   readPtr;
    bool                    writeDone;
    bool                    readDone;
    int                     writeCnt;
    int                     readCnt;
    std::mutex              mut;
    std::condition_variable condVar;
    bool                    ready;

    void                  tryToRotate();
};

#endif
