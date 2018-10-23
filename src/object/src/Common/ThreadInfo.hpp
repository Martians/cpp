
#pragma once

#include "Common/Time.hpp"

namespace common {

	/**
	 * thread info struct
	 **/
	class ThreadInfo
	{
	public:
		ThreadInfo(int _index, int _type = TT_normal, const char*_name = "", int64_t _tid = 0);

		enum ThreadType {
			TT_normal = 0,
			TT_main,
			TT_pool,
		};

	public:
		/**
		 * do time check
		 **/
		ctime_t	check() { return  m_time.check(); }

		/**
		 * get last time
		 **/
		ctime_t	last() { return  m_time.last(); }

		/**
		 * check if thread need record time
		 **/
		bool	record() { return m_type != TT_normal; }

	public:
		/** global thread index */
		int			m_index = {0};
		/** thread type */
		int		 	m_type = {TT_normal};
		/** current name */
		char 		m_name[32] = {0};
		/** current pthread id */
		int64_t 	m_pid = {0};
		/** thread timer */
		TimeRecord  m_time;
	};

	/**
	 * get current thread info
	 **/
	ThreadInfo* thread_info();

	/**
	 * reset thread info
	 **/
	void	reset_thread(const char* name = "");

	/**
	 * set current thread info
	 **/
	void	set_thread(const char* name = "", int type = ThreadInfo::TT_normal);

	/**
	 * set pool thread info
	 **/
	inline void	pool_thread(const char* name) { set_thread(name, ThreadInfo::TT_pool); }

	/**
	 * get current thread name
	 * */
	inline const char* thread_name() {
		ThreadInfo* thread = thread_info();
		return thread ? thread->m_name : NULL;
	}

	/**
	 * reset thread timer
	 * */
	inline void	time_reset() {
		ThreadInfo* thread = thread_info();
		if (thread) {
			thread->check();
		}
	}

	/**
	 * get thread last time
	 * */
	inline ctime_t time_last(bool check = false) {
		ThreadInfo* thread = thread_info();
		if (thread && thread->record()) {
			return check ? thread->check() : thread->last();
		}
		return 0;
	}
}


