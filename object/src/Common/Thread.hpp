
#pragma once

//#define UNUSED_ATTR __attribute__((unused))

#include <pthread.h>

namespace common {

	class ThreadCancelDisabler
	{
	public:
		ThreadCancelDisabler() {
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
		}

		~ThreadCancelDisabler() {
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		}
	};

	/**
	 * common thread base
	 **/
	class CommonThread {
	public:
		CommonThread(const char* name = "", int type = 0) {
			set_name(name, type);
		}
		virtual ~CommonThread() {}

	public:
		typedef void* (thread_handle_t)(void* arg);

		/**
		 * get local thread id
		 **/
		pthread_t &get_thread_id() { return m_thread_id; }

		/**
		 * check if thread is started
		 **/
		bool 	is_started() { return m_thread_id != 0; }

		/**
		 * check if work in thread now
		 **/
		bool 	am_self() { return (pthread_self() == m_thread_id); }

		/**
		 * set thread name and type
		 **/
		void 	set_name(const char* name = "", int type = 0);

		//static int get_num_threads() { return _num_threads.test(); }

		/**
		 * create thread
		 **/
		int 	create(bool detach = false, thread_handle_t handle = NULL, void* arg = NULL);

		/**
		 * join thread
		 **/
		int 	join(void **prval = 0);

		/**
		 * detach
		 **/
		int 	detach();

		/**
		 * send signal
		 **/
		int 	kill(int signal);

		/**
		 * cancel current thread
		 **/
		void 	cancel() {
			if (m_thread_id) {
				pthread_cancel(m_thread_id);
			}
		}

	protected:
		/**
		 * thread entry, if you will not give a thread_handle_t, you should inherit this when start
		 **/
		virtual void *entry() { assert(0); }

		static void *_entry_func(void *arg) {
			CommonThread* thread = (CommonThread*)arg;
			thread->set_name();
			return thread->entry();
		}

	private:
		/** thread id */
		pthread_t 	m_thread_id = {0};
		char		m_name[32] = {0};
		int			m_type = {0};
	};
}

#if COMMON_SPACE
	using common::CommonThread;
#endif

