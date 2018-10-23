
#pragma once

#include "Common/Atomic.hpp"

namespace common {

	/**
	 * define base reference code in class
	 * */
	#define REFER_BASE_DEFINE			\
		void 	inc() { m_refer.inc(); }\
		int		refer() {				\
			return m_refer.ref(); 		\
		}								\
		Refer m_refer;

	/**
	 * define base reference code in class
	 * */
	#define REFER_DEFINE				\
		bool 	dec() {					\
			return m_refer.dec();		\
		}								\
		REFER_BASE_DEFINE


	/**
	 * define reference code in class
	 * */
	#define CYCLE_DEFINE				\
		void 	dec() {					\
			if (m_refer.dec()) {		\
				cycle();				\
			}							\
		}								\
		REFER_BASE_DEFINE
}

