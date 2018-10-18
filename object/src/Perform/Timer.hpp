
#pragma once

#include <string>
#include <cassert>

#include "Common/ThreadInfo.hpp"
#include "Common/Atomic.hpp"
#include "Common/Time.hpp"

namespace common {
namespace tester {

	/**
	 * record stadge timer
	 * */
	struct StadgeTimer {

	public:
		/**
		 * stadge item
		 **/
		struct Item {
			/** time point */
			common::ctime_t time = {0};
			/** bomb time limit */
			int			bomb = {0};
			/** display name */
			const char*	name = {NULL};
		};

		/**
		 * reset timer
		 * @param last keep last time or not
		 **/
		void	reset(bool last = false, int bomb = 0) {
			if (last && m_index >= 1) {
				m_time = m_item[m_index - 1].time;

			} else {
				m_time = common::ctime_now();
			}
			m_index = 0;
			m_curr = -1;
			m_bomb = bomb == 0 ? s_bomb : bomb;
		}

		/**
		 * move to next
		 **/
		void	next(const char* name = NULL, int bomb = 0) {
			if (m_index >= s_count) {
				assert(0);
				return;
			}
			int index = at_inc(m_index);
			m_item[index].name = name;
			m_item[index].bomb = bomb;
			m_item[index].time = common::ctime_now();
		}

	public:
		/**
		 * get time by index
		 **/
		Item*	operator [] (int index) {
			return valid(index) ? &m_item[index] : NULL;
		}

		/**
		 * get index time
		 **/
		common::ctime_t	time(int index) {
			if (valid(index)) {
				common::ctime_t last = (index == 0) ? m_time
					: m_item[index - 1].time;
				return m_item[index].time - last;
			}
			return -1;
		}

		/**
		 * get index time
		 **/
		 common::ctime_t time(Item* item) {
			return time(item - m_item);
		}

		/**
		 * prepare for traverse
		 **/
		void	init() { m_curr = -1; }

		/**
		 * loop next
		 **/
		Item*	get() {
			if (m_curr == -1) {
				m_curr = 0;
			}
			if (valid(m_curr)) {
				return &m_item[m_curr++];

			} else {
				return NULL;
			}
		}

		/**
		 * reverse loop next
		 **/
		Item*	rget() {
			if (m_curr == -1) {
				m_curr = m_index - 1;
			}
			if (valid(m_curr)) {
				return &m_item[m_curr--];

			} else {
				return NULL;
			}
		}

		/**
		 * get string
		 **/
		std::string display() {
			std::string value;
			return string(value);
		}

		/**
		 * check if need dump time
		 **/
		bool		bomb(const char* prefix = NULL);

		/**
		 * display string
		 **/
		void		dump(const char* prefix = "");

	protected:
		/**
		 * get display string
		 **/
		std::string& string(std::string& value);

		/**
		 * check if index is valid
		 **/
		bool		valid(int index) { return index >= 0 && index < m_index; }

	public:
		static const int s_count = 20;
		static const int s_bomb = 1000;
		/** default bomb time */
		int			m_bomb = {s_bomb};
		/** start time */
		common::ctime_t	m_time = {common::ctime_now()};
		/** item array */
		Item	 	m_item[s_count];
		/** total item count */
		int			m_index = {0};
		/** used for loop */
		int			m_curr  = {-1};
	};

	/**
	 * display thread last using
	 * */
	std::string	time_using(bool mute_warn = false);

	/**
	 * check if thread need bombed
	 **/
	bool		time_bombed();

	/**
	 * macro for time trace
	 * */
	/** dump time everytime */
	#define time_log_using(x, log) 		common::time_reset(); log(x << common::tester::time_using())
	#define time_trace_using(x)			time_log_using(x, log_trace)
	#define time_debug_using(x)			time_log_using(x, log_debug)
	/** never dump warning */
	#define time_log_mute_warn(x, log) 	time_reset(); log(x << common::tester::time_using(true))
	#define time_trace_mute_warn(x)		time_log_mute_warn(x, log_trace)
	#define time_debug_mute_warn(x)		time_log_mute_warn(x, log_debug)
	/** only dump warning */
	#define	time_bomb(x) 				if (common::tester::time_bombed()) { log_debug(x << common::tester::time_using()); }

	/**
	 * macro for time record
	 **/
	#define CREATE_TIMER	common::TimeRecord timer
	#define	UPDATE_TIMER	timer.check();
	#define EXPORT_TIMER	" use " << common::string_record(timer)
	#define	THREAD_TIMER(x)	common::tester::thread_wait(); log_debug(x << EXPORT_TIMER)

	#define STADGE_TIMER	common::StadgeTimer stadge
	#define	STADGE_UPDATE	stadge.next()
	#define STADGE_STRING	common::string_timer(stadge.rget())
}
}
