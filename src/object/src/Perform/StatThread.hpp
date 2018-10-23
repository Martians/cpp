
#pragma once

#include "Common/ThreadBase.hpp"
#include "CodeHelper/Bitset.hpp"
#include "Advance/Functional.hpp"
#include "Perform/Statistic.hpp"

namespace common {
namespace tester {

	/**
	 * task list
	 **/
	class OutputList : public HandleList {
	public:
		typedef std::function<std::string()> display_t;

		/**
		 * regist display handle
		 **/
		template<class Handle, class ... Types>
		void 	add(const char* format, Handle&& handle, Types&&...args) {
			display_t display = std::bind(std::forward<Handle>(handle), std::forward<Types>(args)...);
			task_t task = std::bind(&OutputList::wrap, this, format, std::forward<display_t>(display));
			HandleList::add(task);
		}

		/**
		 * expand handle
		 **/
		const std::string& work() {
			m_local.clear();
			HandleList::work();
            return m_local;
		}

	//protected:
		/**
		 * wrap format handle
		 **/
		void	wrap(const char* format, display_t& handle) {
			::common::append(m_local, format, handle().c_str());
		}

	protected:
		 /** local display buffer */
		 std::string m_local;
	};

	/**
	 * statistic thread for timely output
	 *
	 * @note way1, overwrite entry, and use new display and summary
	 * @note way2, clear and regist again
	 **/
	class StatisticThread : public common::ThreadBase
	{
	public:
		StatisticThread(int32_t wait = 0, int32_t summary = 0) { set(wait, summary); }

		enum Const {
			c_wait_time = 1000,
			c_summary_time = 60000,
		};

	public:
		/**
		 * set timer
		 **/
		void	set(int32_t wait = c_wait_time, int32_t summary = 0) {
			set_wait(wait == 0 ? c_wait_time : wait);
			m_waited = (int64_t)m_wait.tv.tv_sec * c_time_level[1] + m_wait.tv.tv_usec;

			m_check.set(summary == 0 ? c_summary_time : 0);
		}

		/**
		 * restart work
		 **/
		void	restart() {
			reset();
			record(true);
		}

		/**
		 * start record or stop
		 **/
		void	record(bool set);

		/**
		 * output empty or not
		 **/
		void	output_empty(bool set) { set_bit(T_empty, set); }

		/**
		 * get statistic
		 **/
		IOStatic* statis() { return &m_statis; }

		/**
		 * get statis unit
		 **/
		StatisUnit* unit(size_t index, size_t level = 0) {
			if (level > m_array.size()) {
				assert(level <= c_length_1K);
				m_array.resize(level);
			}
			if (index > m_array[level].size()) {
				assert(index <= c_length_1K);
				m_array[level].resize(index);
			}
			return &m_array[level][index];
		}

	public:
		/**
		 * regist default handle
		 **/
		void	regist();

		/**
		 * add output handle
		 **/
		template<class Handle, class ... Types>
		void 	add(const char* format, Handle&& handle, Types&&...args) {
			m_output.add(format, std::forward<Handle>(handle), std::forward<Types>(args)...);
		}

		/**
		 * add summary handle
		 **/
		template<class Handle, class ... Types>
		void 	sum(const char* format, Handle&& handle, Types&&...args) {
			m_summary.add(format, std::forward<Handle>(handle), std::forward<Types>(args)...);
		}

	protected:
		/**
		 * local tag
		 **/
		enum Tag {
			T_null,
			/** record start */
			T_record,
			/** not output */
			T_output,
			/** output empty or not */
			T_empty,
		};
		BITSET_DEFINE;

		/**
		 * thread entry
		 **/
		virtual void *entry();

		/**
		 * reset state
		 **/
		void	reset() {
			set_bit(T_record, 0);
			set_bit(T_empty, 0);

			m_empty.reset();
		}

		/**
		 * do output work
		 **/
		void 	display();

		/**
		 * output summary
		 *
		 * @param force output even if not timeout
		 **/
		void	summary(bool force = false);

		/**
		 * output display
		 **/
		void	output(bool summary);

		/**
		 * increase count, set empty flag
		 **/
		void	active(int64_t count);

		/**
		 * get waited configed interval
		 **/
		int64_t	waited_time() { return m_waited; }

		/**
		 * get empty total count
		 **/
		int64_t	empty_total() {
			m_empty.loop();
			return m_empty.total();
		}

	protected:
		typedef std::vector<StatisUnit> array_single;
		typedef std::vector<array_single> unit_array;
		/** empty count */
		StatisUnit m_empty;
		/** waited time */
		int64_t m_waited = { 0 };
		/** time record */
		TimeRecord m_timer;
		/** time for output summary */
		TimeCheck m_check;
		/** statistic data */
		IOStatic m_statis;
		/** unit array */
		unit_array m_array;
		/** output list */
		OutputList m_output;
		/** summary list */
		OutputList m_summary;
	};
	SINGLETON(StatisticThread, GetStatisThread)

	#define g_statis_thread common::tester::GetStatisThread()
	#define g_statis (*common::tester::GetStatisThread().statis())

	/**
	 * get global statistic unit
	 **/
	inline StatisUnit* global_stunit(size_t index, size_t level = 0) {
		return g_statis_thread.unit(index, level);
	}
}
}
