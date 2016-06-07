/*
 * File:   Time.h
 * Author: lpiccoli
 *
 * Created on September 10, 2010, 11:25 AM
 */

#ifndef _TIMEUTIL_H
#define	_TIMEUTIL_H

#include <string>
#include <time.h>

class TimeTest;
class TimeAverageTest;

class Time {
public:
    Time();
    Time(const Time& other);

    Time operator+(Time other);
    Time operator-(Time other);
    long toMillis();
    long toMicros();
    void now();
    void show();

    friend class TimeTest;
    friend class TimeAverageTest;

private:
    struct timespec _time;
};


/**
 * Helper class for keeping averages and making it easy to collect
 * elapsed times. Times are kept in microseconds.
 *
 * @author L.Piccoli
 */
class TimeAverage {
public:
    TimeAverage(int samples = 60, std::string name = "Processing Time");
    ~TimeAverage();

    void start();
    long end();

    long getMax();
    long getMin();
    long getAverage();
    long getRate();

    void show(std::string leadingSpaces = "");
    void clear();
    void restart();

    friend class TimeAverageTest;
    
private:
    void update();

    /** Name for this time average (for display purposes only) */
    std::string _name;

    /** Buffer that holds a number of time measurements */
    long *_buffer;

    /** Buffer size */
    int _size;

    /** Points to the next element to receive the measurement */
    int _head;

    /** Points to the last element that received a measurement */
    int _tail;

    /** Holds the maximum elapsed time collected */
    long _maxTime;

    /** Holds the minimum elapsed time collected */
    long _minTime;

    /** Moving average - computed every time a new measurement is added */
    long _averageTime;

    /** Sum of the current measurements */
    long _sum;

    /** Track the time when a measurement started */
    Time _start;

    /** Last elapsed time, calculated by the end() method */
    long _elapsedTime;

    /**
     * Checked by the end() method to make sure the timer was started using
     * start() 
     */
    bool _started;

    /** Count how many times start() was called without calling end() first */
    long _falseStartCount;

    /** Count the number of samples taken */
    long _sampleCount;

    /** Count how many times end() was called without start() first */
    long _endFailedCount;

    /**
     * Count how many measurements are 3x above the average. This starts to
     * be counted after the buffer gets full
     */
    unsigned long _aboveAverageCount;

    bool _full;
};

#endif
