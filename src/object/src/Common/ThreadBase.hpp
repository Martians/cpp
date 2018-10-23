
#pragma once

#include "Common/Mutex.hpp"
#include "Common/Cond.hpp"
#include "Common/Time.hpp"
#include "Common/Thread.hpp"
#include "CodeHelper/RunStat.hpp"

namespace common {

typedef void (*timer_work_t)();

class ThreadBase : public CommonThread
{
public:
	ThreadBase(int time_ms = 0, timer_work_t handle = NULL)
		: m_running(false), m_mutex("client thread", true), m_handle(handle)
	{
		set_wait(time_ms);
	}

public:
    /**
     * get mutex
     **/
    Mutex&	mutex() { return m_mutex; }

    /**
     * set wait time
     **/
    void	set_wait(uint32_t ms) {
    	if (ms == 0) {
    		ms = 1 * c_time_level[0];
    	}
    	m_wait = utime_t(ms);
    	m_next = m_wait;
    }

    /**
     * start thread
     **/
    int	 	start(int time_ms = 0, timer_work_t handle = NULL) {

    	Mutex::Locker lock(mutex());
    	if (!change_status(true)) {
        	return -1;
        }
        if (time_ms != 0) {
        	set_wait(time_ms);
        }
		m_handle = handle;
        int err = create();
        assert(err == 0);
        return err;
    }

    /**
     * stop thread
     **/
    int		stop() {
    	do {
        	Mutex::Locker lock(mutex());
        	if (!change_status(false)) {
        		return -1;
        	}
    	} while (0);

        wakeup();
        join();
        return 0;
    }

    /**
     * wait for timeout
     **/
    void	wait(uint32_t time = 0) {
		Mutex::Locker lock(m_mutex);
		wait_unlock(time);
    }

    /**
     * wait for timeout without lock
     **/
    void	wait_unlock(uint32_t time = 0) {
    	if (running()) {
			m_cond.wait_interval(m_mutex,
				time == 0 ? m_next : utime_t(time));
			m_next = m_wait;
    	}
    }

    /**
     * wakup thread for stop
     **/
    void	wakeup() {
    	Mutex::Locker lock(m_mutex);
		wakeup_unlock();
	}

    /**
     * set next wait time
     **/
    void	wake_next(int time) {
    	m_next.set(time);
    }

    /**
     * wakeup lock
     **/
    void	wakeup_unlock() {
    	m_cond.signal_all();
    }

protected:
    /**
     * thread process
     **/
    virtual void *entry() {
    	if (!m_handle) {
    		assert(0);
    	}

		while (waiting()) {
			m_handle();
		}
		return NULL;
	}

    /**
     * check if need wait
     **/
    bool	waiting() {
    	if (running()) {
    		wait();
    	}
    	return running();
    }

    RUNSTATE_DEFINE;

protected:
    Mutex 	m_mutex;
    Cond 	m_cond;
    utime_t	m_wait;
    /** next wait time */
    utime_t	m_next;
    timer_work_t m_handle;
};

/**
 * used for test waiting
 **/
void main_loop();

}
