
#pragma once

#include <thread>
#include <vector>

#include "Common/Logger.hpp"
#include "Common/Mutex.hpp"
#include "Common/ThreadInfo.hpp"

namespace common {
namespace tester {
	/**
	 * set random seed
	 **/
	inline void set_random(int seed = 0) {
		::srandom(seed == 0 ? ::time(NULL) : seed);
	}

	/**
	 * thread vectro for testing
	 **/
	class ThreadVector : public std::vector<std::thread*>
	{
	public:
		ThreadVector() : m_mutex("thread vector") {}
		virtual ~ThreadVector() {
			wait();
		}

	public:
		/**
		 * push thread
		 **/
		void	push(std::thread* thread) {
			common::Mutex::Locker lock(m_mutex);
			push_back(thread);
		}

		/**
		 * wait until all thread done
		 **/
		static void wait() {
			common::Mutex::Locker lock(s_pool.m_mutex);
			for (auto& thread : s_pool) {
				if (thread->joinable()) {
					thread->join();
				}
				delete thread;
			}
			s_pool.clear();
		}

		static ThreadVector s_pool;

	protected:
		common::Mutex m_mutex;
	};

	/** wait all thread done */
	inline void thread_wait() {
		ThreadVector::wait();
		common::reset_thread("pool");
	}

	template<class Handle, class ... Types>
	inline void thread_wrap(Handle handle, Types...args) {
		set_thread("pool", common::ThreadInfo::TT_pool);
		handle(args...);
	};

	template<class Handle, class ... Types>
	void single(Handle handle, Types...args) {
		std::thread* thread = new std::thread(thread_wrap<Handle, Types...>, handle, args...);
		//std::thread* thread = new std::thread(args...);
		ThreadVector::s_pool.push(thread);
	}

	template<class ... Types>
	void batchs(int count, Types...args) {
		for (int i = 0; i < count; i++) {
			single(args...);
		}
	}
}
}

