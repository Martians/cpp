
#pragma once

#include <time.h>
#include <pthread.h>

#include "Common/Mutex.hpp"
#include "Common/Time.hpp"

namespace common {

class Cond {
	/** don't allow copying */
private:
    void operator = (Cond &C) {}
    Cond (const Cond &C) {}

public:
    Cond(Mutex* lock = NULL) : _mutex(lock) {
        int DECLARE_UNUSED_PARAM(r);
        r = pthread_condattr_init(&_attr);
        assert(r == 0);
        r = pthread_condattr_setclock(&_attr, CLOCK_MONOTONIC);
        assert(r == 0);
        r = pthread_cond_init(&_c, &_attr);
        assert(r == 0);
    }

    virtual ~Cond() {
        pthread_cond_destroy(&_c);
        pthread_condattr_destroy(&_attr);
    }

public:
    void	set(Mutex* lock) {
    	_mutex = lock;
    }

    void	reset() {
    	_wakeup = false;
    }

#if 0
    /** mostly, we will lock outside and protect other resource */
    int		wait() {
    	//Mutex::Locker lock(_mutex);
		return wait(*_mutex);
	}

    int 	wait_interval(int ms = 1000) {
    	//Mutex::Locker lock(_mutex);
    	return wait_interval(*_mutex, ms);
    }
#endif

public:
    int 	wait(Mutex &mutex) {
    	if (_wakeup) {
			_wakeup = false;
			return 0;
		}
        int r = pthread_cond_wait(&_c, &mutex._m);
        _wakeup = false;
        return r;
    }

    int 	wait_interval(Mutex &mutex, utime_t interval) {
    	if (_wakeup) {
    		_wakeup = false;
			return 0;
		}
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ts.tv_sec += interval.sec();
        ts.tv_nsec += interval.nsec();

        if (ts.tv_nsec > c_time_level[2]) {
            ts.tv_nsec -= c_time_level[2];
            ts.tv_sec += 1;
        }

        int r = pthread_cond_timedwait(&_c, &mutex._m, &ts);
        _wakeup = false;
        return r;
    }

    int 	wait_interval(Mutex &mutex, int ms) {
    	return wait_interval(mutex, utime_t(ms));
    }

    int 	signal() {
    	return signal_all();
    }

    int 	signal_one() {
    	if (_wakeup) {
    		return 0;
    	}
    	_wakeup = true;
        int r = pthread_cond_signal(&_c);
        return r;
    }

    int 	signal_all() {
    	_wakeup = true;
        //int r = pthread_cond_signal(&_c);
        int r = pthread_cond_broadcast(&_c);
        return r;
    }

protected:
    bool	_wakeup = {false};
    Mutex*	_mutex  = {NULL};
    pthread_cond_t _c;
    pthread_condattr_t _attr;
};
}

#if COMMON_SPACE
	using common::Cond;
#endif


