
#pragma once

namespace common {

	/**
	 * define status code in class
	 **/
	#define	RUNSTATE_DEFINE				\
		bool running() {				\
			return m_running;			\
		}								\
		void running(bool set) {		\
			m_running = set;			\
		}								\
		bool change_status(bool run) {	\
			if (run == m_running) {		\
				return false;			\
			}							\
			m_running = run;			\
			return true;				\
		}								\
		bool m_running = {false}
}

