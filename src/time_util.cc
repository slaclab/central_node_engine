/*
 * File:   Time.cc
 * Author: lpiccoli
 *
 * Created on September 10, 2010, 11:31 AM
 */

#include <iostream>
#include <cmath>

#include "time_util.h"

/**
 * Default constructor, which sets the itself to the current time.
 * 
 * @author L.Piccoli
 */
Time::Time() {
    // now();
}

/**
 * Copy constructor.
 *
 * @param other Time to be copied from.
 * @author L.Piccoli
 */
Time::Time(const Time& other) {
    _time = other._time;
}

/**
 * Adds the specified time to this time and return the result as Time.
 *
 * @param other Time to be added
 * @return result of adding this Time to the other Time
 * @author L.Piccoli
 */
Time Time::operator+(Time other) {
    Time temp;

    if ((_time.tv_nsec + other._time.tv_nsec) > 1000000000) {
        temp._time.tv_sec = _time.tv_sec + other._time.tv_sec + 1;
        temp._time.tv_nsec = _time.tv_nsec + other._time.tv_nsec - 1000000000;
    } else {
        temp._time.tv_sec = _time.tv_sec + other._time.tv_sec;
        temp._time.tv_nsec = _time.tv_nsec + other._time.tv_nsec;
    }
    return temp;
}

/**
 * Calculates the difference in nanoseconds between the specified Time and
 * this Time.
 *
 * This is used to measure elapsed time, so the parameter other is the
 * end time. This time is the start time.
 *
 * TODO: review this code!!!
 *
 * @param other usually the measurement end Time.
 * @return difference between this Time and the other Time
 * @author L.Piccoli
 */
Time Time::operator-(Time other) {
    Time temp;

    time_t secDiff = 0;

    secDiff = _time.tv_sec - other._time.tv_sec;
    if (other._time.tv_nsec < _time.tv_nsec) {
        temp._time.tv_nsec = _time.tv_nsec - other._time.tv_nsec;
    } else {
        secDiff--;
        temp._time.tv_nsec = 1000000000 + _time.tv_nsec - other._time.tv_nsec;
    }
    temp._time.tv_sec = secDiff;
    return temp;

    /*

        if (other._time.tv_sec > _time.tv_sec) {
            secDiff = other._time.tv_sec - _time.tv_sec;
        } else {
            secDiff = _time.tv_sec - other._time.tv_sec;
        }

        if (secDiff == 0) {
            if (other._time.tv_nsec > _time.tv_nsec) {
                temp._time.tv_nsec = other._time.tv_nsec - _time.tv_nsec;
            } else {
                temp._time.tv_nsec = _time.tv_nsec - other._time.tv_nsec;
            }
        } else {


            if ((other._time.tv_nsec - _time.tv_nsec) < 0) {
                secDiff--;
                temp._time.tv_nsec = 1000000000 + other._time.tv_nsec - _time.tv_nsec;
            } else {
                temp._time.tv_nsec = other._time.tv_nsec - _time.tv_nsec;
            }
        }
        temp._time.tv_sec = secDiff;
     */
    //    return temp;
}

long Time::toMillis() {
    long millis = _time.tv_sec * 1000;
    millis += _time.tv_nsec * 1e-6;
    return millis;
}

long Time::toMicros() {
    long micros = _time.tv_sec * 1000000;
    micros += _time.tv_nsec * 1e-3;
    return micros;
}

/**
 * Set the time to the current time.
 *
 * @author L.Piccoli
 */
void Time::now() {
  clock_gettime(CLOCK_REALTIME, &_time);
}

void Time::show() {
    std::cout << _time.tv_sec << ":" << _time.tv_nsec << std::endl;
}

#define CIRCULAR_INCREMENT(X) (X + 1) % _size

TimeAverage::TimeAverage(int samples, std::string name) :
_name(name),
_size(samples),
_head(0),
_tail(0),
_maxTime(0),
_minTime(0xfffffff),
_averageTime(0),
_sum(0),
_elapsedTime(0),
_started(false),
_falseStartCount(0),
_sampleCount(0),
_endFailedCount(0),
_full(false) {
    _buffer = new long [samples];
    for (int i = 0; i < samples; ++i) {
        _buffer[i] = 0;
    }
}

TimeAverage::~TimeAverage() {
    delete [] _buffer;
}

void TimeAverage::start() {
    if (_started) {
        _falseStartCount++;
    }

    _start.now();
    _started = true;
}

void TimeAverage::restart() {
    _start.now();
    _started = true;
}

long TimeAverage::end() {
    if (_started) {
        Time end;
        end.now();
        Time difference;
        difference = end - _start;
        _elapsedTime = difference.toMicros();

        update();
        _started = false;
	return _elapsedTime;
    }
    else {
      _endFailedCount++;
    }
    return -1;
}

void TimeAverage::update() {
    if (_elapsedTime > _maxTime) {
        _maxTime = _elapsedTime;
    }

    if (_elapsedTime < _minTime) {
        _minTime = _elapsedTime;
    }

    _sum -= _buffer[_tail];

    _head = CIRCULAR_INCREMENT(_head);
    if (_head == _tail) {
        _full = true;
        _tail = CIRCULAR_INCREMENT(_tail);
    }

    _buffer[_head] = _elapsedTime;
    _sum += _buffer[_head];

    _sampleCount++;
}

long TimeAverage::getMax() {
    return _maxTime;
}

long TimeAverage::getMin() {
    return _minTime;
}

long TimeAverage::getAverage() {
    if (_full) {
        _averageTime = _sum / _size;
    } else {
      _averageTime = 0;
      if (_head != 0) {
        _averageTime = _sum / _head;
      }
    }
    return _averageTime;
}

long TimeAverage::getRate() {
    if (_full) {
        _averageTime = _sum / _size;
    } else {
      if (_head != 0) {
        _averageTime = _sum / _head;
      }
    }
    if (_averageTime != 0) {
      return std::floor(1000000/_averageTime);
    }
    return 0;
}

void TimeAverage::show(std::string leadingSpaces) {
    std::cout << leadingSpaces << "--- " << _name << " ---" << std::endl;
    std::cout << leadingSpaces << "  average: " << getAverage() << " usec (samples="
            << _size << ", total count=" << _sampleCount << ", fs="
	      << _falseStartCount << ", end fail=" << _endFailedCount << ")" << std::endl;
    std::cout << leadingSpaces << "  max:     " << _maxTime << " usec" << std::endl;
    std::cout << leadingSpaces << "  min:     " << _minTime << " usec" << std::endl;
}

void TimeAverage::clear() {
    _head = 0;
    _tail = 0;
    _maxTime = 0;
    _minTime = 0xfffffff;
    _averageTime = 0;
    _sum = 0;
    _elapsedTime = 0;
    _started = false;
    _sampleCount = 0;
    _endFailedCount = 0;
    _full = false;
    for (int i = 0; i < _size; ++i) {
        _buffer[i] = 0;
    }
}

