
#pragma once

#include <sys/time.h>

#include "Common/Type.hpp"

namespace common {

	typedef uint64_t ctime_t;
	const uint32_t c_time_level[3] = { 1000L, 1000000L, 1000000000L };

	inline ctime_t time_up(ctime_t v, int level = 1) { return v / c_time_level[level - 1]; }
	inline ctime_t time_dw(ctime_t v, int level = 1) { return v * c_time_level[level - 1]; }

	/**
	 * get time now
	 **/
	inline ctime_t ctime_now() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return (((ctime_t)tv.tv_sec) * c_time_level[1] + tv.tv_usec);
	}

	/**
	 * record timer
	 **/
	struct TimeRecord
	{
	public:
		TimeRecord() { reset(); }
		TimeRecord(const TimeRecord& v) {
			*this = v;
		}

		const TimeRecord& operator = (const TimeRecord& v) {
			m_time = v.m_time;
			m_curr = v.m_curr;
			m_last = v.m_last;
			return *this;
		}

		class OutputScope;
	public:
		/**
		 * reset timer
		 **/
		void	reset() {
			m_time = m_curr = ctime_now();
			m_last = 0;
		}

		/**
		 * cal time from last record
		 **/
		ctime_t	check() {
			ctime_t now = ctime_now();
			m_last = now - m_curr;
			m_curr = now;
			return m_last;
		}

		/**
		 * start recording time
		 **/
		void	begin() { reset(); }

		/**
		 * set end point
		 **/
		ctime_t	end() { return check(); }

		/**
		 * get time form last record
		 **/
		ctime_t	last() { return m_last; }

		/**
		 * get time form last record
		 **/
		ctime_t	curr() { return m_curr; }

		/**
		 * get elapse time from start record
		 **/
		ctime_t	elapse() { return m_curr - m_time; }

	public:
		/** start time */
		ctime_t m_time;
		/** current time */
		ctime_t m_curr;
		/** last interval */
		ctime_t m_last;
	};

	/**
	 * time util for timeout
	 **/
	struct TimeCheck
	{
	public:
		TimeCheck(int wait_time = 0) { set(wait_time); }

		TimeCheck(const TimeCheck& v) {
			operator = (v);
		}

		const TimeCheck& operator = (const TimeCheck& v) {
			m_wait = v.m_wait;
			m_last = v.m_last;
			return *this;
		}
	public:
		/**
		 * set timeout
		 **/
		void	set(int wait_time = 0) {
			m_wait = wait_time * c_time_level[0];
			reset();
		}

		/**
		 * get wait time in ms
		 **/
		int		wait() { return m_wait / c_time_level[0]; }

		/**
		 * update current time
		 **/
		void	reset() { m_last = ctime_now(); }

		/**
		 * clear last time, trigger timeout rightly
		 **/
		void	wakeup() { m_last = 0; }

		/**
		 * set next wakeup interval
		 **/
		void	next(int wait_time) {
			m_last = ctime_now() + wait_time * c_time_level[0] - m_wait;
		}

		/**
		 * only check but not change m_last
		 **/
		bool	check(ctime_t* _now = NULL) {
			ctime_t now = _now ? *_now : ctime_now();
			if (m_last > now || m_last + m_wait <= now) {
				m_wake = (m_last == 0);
				return true;
			}
			return false;
		}

		/**
		 * check if timeout
		 **/
		bool	expired(ctime_t* _now = NULL) {
			ctime_t now = _now ? *_now : ctime_now();
			if (check(&now)) {
				m_last = now;
				return true;
			}
			return false;
		}

		/**
		 * get last time from last timeout
		 **/
		int		elapse() { return ctime_now() - m_last; }

		/**
		 * get rest time
		 **/
		int		rest(bool reset = true) {
			ctime_t now = ctime_now();
			if (check(&now)) {
				if (reset) {
					m_last = now;
				}
				return 0;
			}
			return (m_last + m_wait - now) / c_time_level[0];
		}

		/**
		 * get last timeout method
		 **/
		bool	waked() { return m_wake; }

	public:
		/** wait time */
		ctime_t m_wait;
		/** last check time */
		ctime_t m_last;
		/** timeout or wakeup */
		bool	m_wake = {false};
	};

	/**
	 * timer counter
	 **/
	class TimeCounter
	{
	public:
		TimeCounter(int duration, int wait = 0) { set(duration, wait); }

		TimeCounter(const TimeCounter& v) {
			operator = (v);
		}

		const TimeCounter& operator = (const TimeCounter& v) {
			m_duration = v.m_duration;
			m_time = v.m_time;
			m_wait = v.m_wait;
			m_rest = v.m_rest;
			return *this;
		}

	public:
		/**
		 * set wait time
		 **/
		void	set(int duration, int wait = 0) {
			m_duration = duration * c_time_level[0];
			m_wait = wait * c_time_level[0];
			m_time = ctime_now();
		}

		/**
		 * get origin step time
		 **/
		int		step() { return m_wait / c_time_level[0]; }

		/**
		 * get timer duration
		 **/
		int		duration() { return m_duration / c_time_level[0]; }

		/**
		 * time counter expired or not
		 **/
		bool	remain() {
			ctime_t now = ctime_now();
			if (m_time > now || m_time + m_duration <= now) {
				return false;
			}
			m_rest = m_duration - (now - m_time);
			return true;
		}

		/**
		 * get current wait time
		 **/
		int		wait() {
            if (m_rest > m_wait) {
                return m_wait / c_time_level[0];
            } else {
			    return m_rest / c_time_level[0];
            }
		}

		/**
		 * get remain wait time
		 **/
		int		rest() { return m_rest / c_time_level[0]; }

	protected:
		/** duration time */
		ctime_t m_duration;
		/** start time */
		ctime_t m_time;
		/** sleep time */
		ctime_t m_wait;
		/** current remain */
		ctime_t m_rest;
	};

	/**
	 * base timer used for system api
	 **/
	class utime_t {
	public:
		struct {
			uint32_t tv_sec, tv_usec;
		} tv;

		friend class Clock;

	public:
		void normalize() {
			if (tv.tv_usec > c_time_level[1]) {
				tv.tv_sec += tv.tv_usec / c_time_level[1];
				tv.tv_usec %= c_time_level[1];
			}
		}

		// cons
		utime_t() {
			tv.tv_sec = 0;
			tv.tv_usec = 0;
			normalize();
		}

		utime_t(time_t s, int u) {
			tv.tv_sec = s;
			tv.tv_usec = u;
			normalize();
		}

		explicit utime_t(uint32_t ms) {
			set(ms);
		}
		/*
		 utime_t(const struct ceph_timespec &v) {
		 	 decode_timeval(&v);
		 }
		 */

		utime_t(const struct timeval &v) {
			set_from_timeval(&v);
		}
		utime_t(const struct timeval *v) {
			set_from_timeval(v);
		}

		const utime_t& operator = (const utime_t& v) {
			tv.tv_sec 	= v.tv.tv_sec;
			tv.tv_usec 	= v.tv.tv_usec;
			return *this;
		}
		/*
		explicit utime_t(const double d) {
			set_from_double(d);
		}
		*/

		void set(uint32_t ms) {
			tv.tv_sec = ms / c_time_level[0];
			tv.tv_usec = (ms % c_time_level[0]) * c_time_level[0];
		}

		void set_from_double(const double d) {
	//    tv.tv_sec = (uint32_t)trunc(d);
			tv.tv_usec = (uint32_t) ((d - (double) tv.tv_sec) * (double) 1000000.0);
		}

		// accessors
		time_t sec() const {
			return tv.tv_sec;
		}
		long usec() const {
			return tv.tv_usec;
		}
		int nsec() const {
			return tv.tv_usec * 1000;
		}

		// ref accessors/modifiers
		uint32_t& sec_ref() {
			return tv.tv_sec;
		}
		uint32_t& usec_ref() {
			return tv.tv_usec;
		}

		void copy_to_timeval(struct timeval *v) const {
			v->tv_sec = tv.tv_sec;
			v->tv_usec = tv.tv_usec;
		}
		void set_from_timeval(const struct timeval *v) {
			tv.tv_sec = v->tv_sec;
			tv.tv_usec = v->tv_usec;
		}
		/*
		 void encode(bufferlist &bl) const {
		 ::encode(tv.tv_sec, bl);
		 ::encode(tv.tv_usec, bl);
		 }
		 void decode(bufferlist::iterator &p) {
		 ::decode(tv.tv_sec, p);
		 ::decode(tv.tv_usec, p);
		 }
		 */
		/*
		 void encode_timeval(struct timespec *t) const {
		 t->tv_sec = tv.tv_sec;
		 t->tv_nsec = tv.tv_usec*1000;
		 }
		 void decode_timeval(const struct timespec *t) {
		 tv.tv_sec = t->tv_sec;
		 tv.tv_usec = t->tv_nsec / c_time_level[0];
		 }
		 */
		// cast to double
		operator double() {
			return (double) sec() + ((double) usec() / c_time_level[1]);
		}
		/*
		 operator timespec() {
		 timespec ts;
		 ts.tv_sec = sec();
		 ts.tv_nsec = nsec();
		 return ts;
		 }
		 */
	};

}

#if COMMON_SPACE
	using common::ctime_t;
	using common::utime_t;
	using common::TimeRecord;
	using common::TimeCounter;
	using common::TimeCheck;
#endif

