#pragma once

#include "Common/Type.hpp"
#include "Common/Define.hpp"
#include "Common/Atomic.hpp"
#include "Common/Time.hpp"
#include "Common/CodeHelper.hpp"

namespace common {
	/**
	 * base statistic unit
	 **/
	struct StatisUnit
	{
	public:
		StatisUnit() : m_count(0), m_total(0), m_last(0) {}

	public:
		/**
		 * reset state
		 * */
		void	reset() {
			m_total = 0;
			m_count = 0;
			m_last  = 0;
		}

		/**
		 * inc by 1
		 **/
		void 	inc() { common::atomic_inc64(&m_count); }

		/**
		 * dec by 1
		 **/
		void	dec() { common::atomic_dec64(&m_count); }

		/**
		 * inc count
		 **/
		int64_t	inc(int64_t v) { return common::atomic_add64(&m_count, v); }

		/**
		 * dec count
		 **/
		int64_t	dec(int64_t v) { return common::atomic_add64(&m_count, -v); }

		/**
		 * loop count
		 **/
		int64_t	loop() {
			m_last = common::atomic_swap64(&m_count, 0);
			m_total += m_last;
			return m_last;
		}

		/**
		 * get last count
		 **/
		int64_t	last() { return m_last; }

		/**
		 * get current count
		 **/
		int64_t	count() { return m_count; }

		/**
		 * get total count
		 **/
		int64_t total() { return m_total; }

	public:
		typedef std::function<int64_t(void)> handle_t;

		/**
		 * get count handle
		 **/
		handle_t hd_loop() { return std::bind(&StatisUnit::loop, this); }

		/**
		 * get count handle
		 **/
		handle_t hd_last() { return std::bind(&StatisUnit::last, this); }

		/**
		 * get count handle
		 **/
		handle_t hd_total() { return std::bind(&StatisUnit::total, this); }

		/**
		 * handle type
		 **/
		enum Handle {
			T_null = 0,
			T_loop,
			T_last,
			T_total,
		};

		/**
		 * get handle by type
		 **/
		handle_t handle(int type) {
			switch (type) {
            case T_loop:	return std::bind(&StatisUnit::loop, this);
				break;
            case T_last:	return std::bind(&StatisUnit::last, this);
				break;
            case T_total:	return std::bind(&StatisUnit::total, this);
				break;
			default:
				break;
			}
		}

	public:
		int64_t m_count;
		int64_t m_total;
		int64_t	m_last;
	};

	/**
	 * time statistic struct
	 **/
	struct TimeStatis
	{
	public:
		TimeStatis() {}

		enum {
			TT_count = 0,
			TT_times,
			TT_timer,
		};
	public:
		/**
		 * reset state
		 * */
		void	reset() {
			m_times.reset();
			m_count.reset();
			m_timer.reset();
		}

		/**
		 * inc count
		 **/
		void	inc(ctime_t time) {
			inc(1, time);
		}
		/**
		 * dec count
		 **/
		void	dec(ctime_t time) {
			dec(1, time);
		}

		/**
		 * inc count
		 **/
		void	inc(int64_t v, ctime_t time) {
			m_times.inc();
			m_count.inc(v);
			m_timer.inc(time / c_time_level[0]);
		}

		/**
		 * dec count
		 **/
		void	dec(int64_t v, ctime_t time) {
			m_times.dec();
			m_count.dec(v);
			m_timer.dec(time / c_time_level[0]);
		}

		/**
		 * loop count
		 **/
		void	loop() {
			m_times.loop();
			m_count.loop();
			m_timer.loop();
		}

		/**
		 * get inner statistic
		 **/
		StatisUnit& stat(int type = 0) {
			switch (type) {
			case TT_times:	return m_times; break;
			case TT_count:	return m_count; break;
			case TT_timer:	return m_timer; break;
			default:{ assert(0); return m_times; }
				break;
			}
		}

	public:
		/** record times */
		StatisUnit m_times;
		/** count increase */
		StatisUnit m_count;
		/** timer counter */
		StatisUnit m_timer;
	};

	/**
	 * statistic used for iops and size
	 **/
	struct IOStatic
	{
	public:
		IOStatic() {}

	public:
		/**
		 * reset inner state
		 **/
		void	reset(int64_t _total) {
			iops.reset();
			size.reset();
			warn.reset();
			total = _total;
		}

		/**
		 * inc iops
		 **/
		void	inc_ipos(int64_t v = 1) { iops.inc(v); }

		/**
		 * inc size
		 **/
		void	inc_size(int64_t v = 1) { size.inc(v); }

		/**
		 * inc error
		 **/
		void	inc_warn(int64_t v = 1) { warn.inc(v); }

		/**
		 * inc iops and length
		 **/
		void	inc(int64_t count, int64_t length, int64_t warning = 0) {
			if (warning == 0) {
				iops.inc(count);
				size.inc(length);
			} else {
				warn.inc(warning);
			}
		}

		/**
		 * loop and get last iops and size
		 **/
		void	Loop(int64_t& last_iops, int64_t& last_size, int64_t& last_warn) {
			last_iops = iops.loop();
			last_size = size.loop();
			last_warn = warn.loop();
		}

	public:
		/** iops statistic */
		StatisUnit	iops;
		/** size statistic */
		StatisUnit	size;
		/** error code */
		StatisUnit	warn;
		/** total count */
		int64_t 	total = {1};
	};

	inline StatisUnit& global_statis(int index, int type) {
		const int c_index = 10; const int c_type = 30;
		static StatisUnit unit[c_index][c_type];
		return unit[index][type];
	}
	inline TimeStatis& global_timers(int index, int type) {
		const int c_index = 3; const int c_type = 30;
		static TimeStatis time[c_index][c_type];
		return time[index][type];
	}

	/** time function */
	#define TIME_FUNC_STAT(name, index)						\
		inline StatisUnit& name##_stat(int type, int stat = 0) {\
		return global_timers(index, type).stat(stat);		\
	}
	#define TIME_FUNC_INC(name, index)						\
		inline void name##_inc(int type, common::ctime_t time) {\
		global_timers(index, type).inc(time);				\
	}
	#define TIME_FUNC_DEC(name, index)						\
		inline void name##_dec(int type, common::ctime_t time) {\
		global_timers(index, type).dec(time);				\
	}
	#define TIME_FUNC_INC_COUNT(name, index)				\
		inline void name##_inc(int type, int64_t count, common::ctime_t time) {	\
		global_timers(index, type).inc(count, time);		\
	}
	#define TIME_FUNC_DEC_COUNT(name, index)				\
		inline void name##_dec(int type, int64_t count, common::ctime_t time) {	\
		global_timers(index, type).dec(count, time);		\
	}
	#define TIME_FUNC_LOOP(name, index)						\
		inline void name##_loop(int type) {					\
		global_timers(index, type).loop();					\
	}
	#define TIME_FUNC_LOOP_RANGE(name, index)				\
		inline void name##_loop(int beg, int end) {			\
		for (int type = beg; type <= end; type++) {			\
			global_timers(index, type).loop();				\
		}													\
	}
	#define DEFINE_TIME_FUNC(name, index)		\
			TIME_FUNC_STAT(name, index)			\
			TIME_FUNC_INC(name, index)			\
			TIME_FUNC_DEC(name, index)			\
			TIME_FUNC_INC_COUNT(name, index)	\
			TIME_FUNC_DEC_COUNT(name, index)	\
			TIME_FUNC_LOOP(name, index)			\
			TIME_FUNC_LOOP_RANGE(name, index)

	/** stat function */
	#define STAT_FUNC_COUNT(name, index)				\
		inline int64_t name##_count(int type) {			\
		return global_statis(index, type).count();		\
	}
	#define STAT_FUNC_COUNT_RANGE(name, index)			\
		inline int64_t name##_count(int beg, int end) {	\
		int64_t	total = 0;								\
		for (int type = beg; type <= end; type++) {		\
			total += global_statis(index, type).count();\
		}												\
		return total;									\
	}
	#define STAT_FUNC_LAST(name, index)					\
		inline int64_t name##_last(int type) {			\
		return global_statis(index, type).last();		\
	}
	#define STAT_FUNC_LAST_RANGE(name, index)			\
		inline int64_t name##_last(int beg, int end) {	\
		int64_t	total = 0;								\
		for (int type = beg; type <= end; type++) {		\
			total += global_statis(index, type).last();	\
		}												\
		return total;									\
	}
	#define STAT_FUNC_INC(name, index)					\
		inline void name##_inc(int type) {				\
		global_statis(index, type).inc();				\
	}
	#define STAT_FUNC_DEC(name, index)					\
		inline void name##_dec(int type) {				\
		global_statis(index, type).dec();				\
	}
	#define STAT_FUNC_INC_COUNT(name, index)			\
		inline void name##_inc(int type, int64_t count) {\
		global_statis(index, type).inc(count);			\
	}
	#define STAT_FUNC_DEC_COUNT(name, index)			\
		inline void name##_dec(int type, int64_t count) {\
		global_statis(index, type).dec(count);			\
	}
	#define STAT_FUNC_LOOP(name, index)					\
		inline int64_t name##_loop(int type) {			\
		global_statis(index, type).loop();				\
		return global_statis(index, type).last();		\
	}
	#define STAT_FUNC_LOOP_RANGE(name, index)			\
		inline void name##_loop(int beg, int end) {		\
		for (int type = beg; type <= end; type++) {		\
			global_statis(index, type).loop();			\
		}												\
	}
	#define STAT_FUNC_TOTAL(name, index)				\
		inline void name##_total(int type) {			\
		global_statis(index, type).total();				\
	}
	#define STAT_FUNC_NEXT(name, index, step)			\
		inline void name##_next(int type) {				\
		global_statis(index, type + step).dec();		\
		global_statis(index, type).inc();				\
	}
	#define STAT_FUNC_NEXT_COUNT(name, index, step)		\
		inline void name##_next(int type, int64_t count) {\
		global_statis(index, type + step).dec(count);	\
		global_statis(index, type).inc(count);			\
	}
	#define DEFINE_STAT_FUNC(name, index, step)	\
			STAT_FUNC_COUNT(name, index)		\
			STAT_FUNC_COUNT_RANGE(name, index)	\
			STAT_FUNC_LAST(name, index)			\
			STAT_FUNC_LAST_RANGE(name, index)	\
			STAT_FUNC_INC(name, index)			\
			STAT_FUNC_DEC(name, index)			\
			STAT_FUNC_INC_COUNT(name, index)	\
			STAT_FUNC_DEC_COUNT(name, index)	\
			STAT_FUNC_LOOP(name, index)			\
			STAT_FUNC_TOTAL(name, index)		\
			STAT_FUNC_LOOP_RANGE(name, index)	\
			STAT_FUNC_NEXT(name, index, step)	\
			STAT_FUNC_NEXT_COUNT(name, index, step)

	/*
	 *	enum {
	 *		GT_file = 1,
	 *		GT_update,
	 *		GT_range,
	 *	};
	 *	DEFINE_STATA_FUNC(update, GT_update, -3)
	 *	DEFINE_STATA_FUNC(file,   GT_file,   0)
	 *	DEFINE_STATA_FUNC(range,  GT_range, -1)
	 */

	/** define statistic enum type */
	#define ENUM_STATIS(code, __)  		\
			Statis_ ## code,
	/** define statistic type func */
	#define FUNC_STATIS(code, __) 	 	\
			DEFINE_STAT_FUNC(code, Statis_ ## code, 1)
	/** define statistic functions */
	#define STATIS_FUNCTIONS(ENUM_MAP)	\
			ENUM_MAP(FUNC_STATIS)
	/** define statis type and static function */
	#define DEFINE_STATIS(Type, ENUM_MAP)					\
			DEFINE_TYPE_ENUM(Type, ENUM_MAP, ENUM_STATIS);	\
			STATIS_FUNCTIONS(ENUM_MAP)

	/** define time enum type */
	#define ENUM_TIMERS(code, __)  		\
			Timers_ ## code,
	#define FUNC_TIMERS(code, __) 	 	\
			DEFINE_TIME_FUNC(code, Timers_ ## code)
	/** define timer functions */
	#define TIMERS_FUNCTIONS(ENUM_MAP)	\
			ENUM_MAP(FUNC_TIMERS)
	/** define statis type and static function */
	#define DEFINE_TIMERS(Type, ENUM_MAP)					\
			DEFINE_TYPE_ENUM(Type, ENUM_MAP, ENUM_TIMERS);	\
			TIMERS_FUNCTIONS(ENUM_MAP)
	//#include "Advance/Define.hpp"
	/*
	 * #define GLOBAL_INDEX(XX)
	 *	XX(GT_update)		\
	 *	XX(GT_status)		\
	 *	XX(GT_file)
	 *
	 * DEFINE_STATIS(GlobalStatus, GLOBAL_INDEX)
	 */

}
