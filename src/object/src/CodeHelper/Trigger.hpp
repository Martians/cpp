
#pragma once

namespace common {
	/**
	 * current function used for initialize, run only once
	 * */
	#define TRIGGER_INIT(x)		 do {	\
		static bool s_once = false;		\
			if (s_once) {				\
                return x; 				\
			}							\
			s_once = true;				\
	} while(0)

	/**
	 * some initialize code in a function, run only once
	 **/
	#define	TRIGGER_ONCE(working) {		\
		static bool s_once = true;		\
		if (s_once) {					\
			s_once = false;				\
			working;					\
		}								\
	}
	/**
	 * some initialize code in a function, run only once
	 **/
	#define	TRIGGER_NEXT(working) {		\
		static bool s_first = true;		\
		if (s_first) {					\
			s_first = false;			\
		} else {						\
			working;					\
		}								\
	}

	/**
	 * trigger check
	 **/
	inline bool trigger_check(int& flag, bool first = true) {
		if (!flag) {
			flag = true;
			return first;
		} else {
			return !first;
		}
	}

	/**
	 * used for debug
	 **/
	#define	TRIGGER_DEC(count, ret) {	\
		static int s_count = count;		\
		if (at_dec(s_count) > 0) {		\
			return ret;					\
		}								\
	}
	#define	TRIGGER_INC(count, ret) {	\
		static int s_count = 0;			\
		if (at_inc(s_count) >= count) {	\
			return ret;					\
		}								\
	}
	#define	TRIGGER_RANGE(min, max, ret) {\
		static int s_count = 0;			\
		int s_curr = at_inc(s_count);	\
		if (s_curr >= min && s_curr < max) {\
			return ret;					\
		}								\
	}
}
