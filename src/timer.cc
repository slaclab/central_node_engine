#include "timer.h"

template<typename T>
Timer<T>::Timer( const std::string& name, size_t size )
:
    size      ( size ),
    name      ( name ),
    TickCount ( 0 ),
    cb        ( size ),
    started   ( false ),
    max       ( 0 )
{
}

template <typename T>
void Timer<T>::start()
{
    t = std::chrono::high_resolution_clock::now();
    started = true;
}

template <typename T>
void Timer<T>::tick()
{
    std::chrono::high_resolution_clock::time_point now =  std::chrono::high_resolution_clock::now();

    if ( started )
    {
        std::chrono::duration<T> diff = now - t;
        cb.push_back( diff.count() );
	if (diff.count() > max) {
	  max = diff.count();
	}
        t = now;
        ++TickCount;
    }
    else
    {
        t = now;
        started = true;
    }
}

template <typename T>
const T Timer<T>::getMinPeriod()
{
    typename boost::circular_buffer<T>::iterator it = std::min_element(cb.begin(), cb.end());

    if (it != cb.end())
        return *it;
    else
        return -1;}

template <typename T>
const T Timer<T>::getMaxPeriod()
{
    typename boost::circular_buffer<T>::iterator it = std::max_element(cb.begin(), cb.end());

    if (it != cb.end())
        return *it;
    else
        return -1;
}

template <typename T>
const T Timer<T>::getMeanPeriod()
{
    T sum = std::accumulate(cb.begin(), cb.end(), 0.0);
    return sum / cb.size();
}

template<typename T>
const int Timer<T>::getTickCount() const
{
    return TickCount;
}

template <typename T>
const T Timer<T>::getAllMaxPeriod()
{
    return max;
}

template<typename T>
void Timer<T>::clear()
{
  max = 0;
}

template<typename T>
void Timer<T>::show()
{
    std::cout << "--- " << name << " ---"                                         << std::endl;
    std::cout << "Minimum period      : " << Timer<T>::getMinPeriod()  * 1e6 << " us" << std::endl;
    std::cout << "Average period      : " << Timer<T>::getMeanPeriod() * 1e6 << " us" << std::endl;
    std::cout << "Maximum period      : " << Timer<T>::getMaxPeriod()  * 1e6 << " us" << std::endl;
    std::cout << "Maximum period (All): " << Timer<T>::getAllMaxPeriod()  * 1e6 << " us" << std::endl;
    std::cout << "Number of ticks     : " << TickCount                                << std::endl;
}


template class Timer<double>;
