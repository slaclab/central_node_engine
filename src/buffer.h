#ifndef _BUFFER_H_
#define _BUFFER_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>

template <typename T>
class DataBuffer
{
public:
    DataBuffer( const size_t& size );
    ~DataBuffer();

    std::vector<T>*          getWritePtr()  const { return writePtr;    };
    std::vector<T>*          getReadPtr()   const { return readPtr;     };
    size_t                   size()         const { return mySize;      };
    void                     doneWriting();
    void                     doneReading();
    bool                     isReadReady()  const { return !readDone;   };
    bool                     isWriteReady() const { return !writeDone;  };
    std::condition_variable* getCondVar()         { return &condVar;    };
    std::mutex*              getMutex()           { return &mut;        };

private:
    size_t                  mySize;
    std::vector<T>          buf0;
    std::vector<T>          buf1;
    std::vector<T>*         writePtr;
    std::vector<T>*         readPtr;
    bool                    writeDone;
    bool                    readDone;
    int                     writeCnt;
    int                     readCnt;
    std::mutex              mut;
    std::condition_variable condVar;

    void                  tryToRotate();
};

#endif
