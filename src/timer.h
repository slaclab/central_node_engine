#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdio.h>
#include <iostream>
#include <chrono>
#include <boost/circular_buffer.hpp>
#include <numeric>

template<typename T>
class Timer
{
public:
    Timer( const std::string& name, size_t size = 1 );
    virtual ~Timer() {};

    void      start();
    void      tick();
    void      show();
    const int getTickCount() const;
    const T   getMinPeriod();
    const T   getMaxPeriod();
    const T   getMeanPeriod();

private:
    int                                            size;
    std::string                                    name;
    int                                            TickCount;
    boost::circular_buffer<T>                      cb;
    std::chrono::high_resolution_clock::time_point t;
};

#endif