
#pragma once

#include <vector>
#include <deque>
#include <tuple>

#include "Common/Time.hpp"
#include "Common/ThreadBase.hpp"
#include "CodeHelper/Refer.hpp"

#define TIME_RECORD 0

#if TIME_RECORD
#	include "Perform/Timer.hpp"
#endif

namespace common {
	typedef void(thread_handle_t)(void* arg);

	class ThreadPool
	{
	public:
		class TaskManage;

		ThreadPool(const char* name = "", TaskManage* manage = NULL)
			: m_taskm(manage) { set_name(name); }

		virtual ~ThreadPool() { stop(); }

		/**
		 * thread pool status
		 **/
		enum {
			ST_null = 0,
			ST_work,
			ST_pause,
			ST_stop,
		};

	public:
		/**
		 * cycle counter
		 **/
		class Cycle
		{
		public:
			Cycle(int64_t limit)
				: m_cycle(limit) {}

		public:
			/**
			 * check if occur cycle
			 **/
			bool	check(bool reset = true) {
				if (++m_count >= m_cycle) {
					if (reset) {
						m_count = 0;
					}
					return true;
				}
				return false;
			}

			/**
			 * reset counter
			 **/
			void	reset() {
				m_count = 0;
			}
        public:
			int64_t	m_cycle = {0};
			int64_t	m_count = {0};
		};

		/**
		 * pool work thread
		 **/
		class Thread : public CommonThread
		{
		public:
			Thread(ThreadPool* pool, int index)
				: m_pool(pool), m_index(index) {}

		public:
			/**
			 * set thread param
			 * @note you should define a member like this
			 * @note can also use as std::tuple<T...>&& param
			 **/
			template<typename...T>
			void 	set(T&&...args) {}

			/**
			 * thread work entry
			 **/
			virtual void *entry() {
				m_pool->thread(this);
				return NULL;
			}

			/**
			 * do pause work
			 * */
			virtual void pause() {}

			/**
			 * time work handle
			 **/
			virtual void handle() { assert(0); }

		public:
			/**
			 * thread index
			 **/
			int		index() { return m_index; }

			/**
			 * get thread pool
			 **/
			ThreadPool* pool() { return m_pool; }

			/**
			 * set timer
			 **/
			void	timer(int wait) { m_timer.set(wait); }

			/**
			 * reset timer
			 **/
			void	reset_timer() { m_timer.reset(); }

		protected:
			/**
			 * get timer
			 **/
			TimeCheck& timer() { return m_timer; }

			/**
			 * get wait timer
			 **/
			int		wait_time() {
				if (m_timer.wait() == 0) {
					return -1;

				} else {
					return m_timer.rest();
				}
			}

			/**
			 * check if need do time work
			 **/
			void	schedule(bool force = false);

			friend class ThreadPool;

		protected:
			/** thread pool */
			ThreadPool* m_pool;
			/** thread index */
			int			m_index;
			/** schedule timer */
			TimeCheck 	m_timer;
			/** cycle counter */
			Cycle		m_cycle = {1};

			#if TIME_RECORD
			StadgeTimer m_times;
			#endif
		};

		/**
		 * base task
		 **/
		class Task
		{
		public:
			Task() {}
            virtual ~Task() {}

            REFER_DEFINE;

		public:
			/**
			 * simple work operator, you can ignore this, and just change work in TaskManage
			 **/
            virtual bool operator ()() {
				assert(0);
				return true;
			}
		};

		/**
		 * task manage
		 **/
		class TaskManage
		{
		public:
			TaskManage() {}

		public:
			/**
			 * malloc new task
			 **/
			virtual Task* malloc(void* context) = 0;

			/**
			 * call by thread pool, when thread pool stop or deny task; should dec task refer,
			 * if error happen, task will call with error
			 **/
			virtual void cycle(Task* task, int eno = 0) = 0;

			/**
			 * work task
			 *
			 * @return if return true, will
			 **/
			virtual void work(Task* task, ThreadPool::Thread* thread) { (*task)(); }
		};

	public:
		/**
		 * set manager
		 **/
		void	manage(TaskManage* manage) { m_taskm = manage; }

		/**
		 * start thread pool
		 **/
		template <class WorkThread = Thread, class...T>
		int		start(int count, T&&...args) {
			Mutex::Locker lock(m_mutex);
			int ret = change_start();
			if (ret <= 0) {
				return ret;
			}
			for (int i = 0; i < count; i++) {
				WorkThread* thread = new WorkThread(this, i);
				//std::make_tuple(std::ref(args)...)
				//thread->set(std::make_tuple(args...));
				thread->set(args...);

				insert_thread(thread);
				int ret = thread->create();
				assert(ret == 0);
			}
			return 0;
		}

		/**
		 * stop thread pool
		 * @param wait all pending request done or not
		 **/
		int		stop(bool wait_pend = false);

		/**
		 * pause thread pool, cancel all task
		 *
		 * @param wait current work done or not
		 * @note will cancel pending request
		 **/
		int		pause(bool wait_curr = false);

		/**
		 * get new task, just input param
		 **/
		int		add(void* context, bool front = true) {
			return add(m_taskm->malloc(context), front);
		}

		/**
		 * add new task entry
		 **/
		int		add(Task* task, bool front = true);

		/**
		 * put task back to queue
		 **/
		int		add_prior(Task* task, bool front = false);

		/**
		 * get current task count
		 **/
		int		count() { return m_curr; }

		/**
		 * get current run count
		 **/
		int		run_count() { return count() - m_prior.size() - m_tasks.size(); }

		/**
		 * get done count
		 **/
		int		done() { return m_done; }

		/**
		 * thread working
		 **/
		void	thread(Thread* thread) {
			while (next(thread)) {
				thread->schedule();
			}
		}

		/**
		 * set thread pool name
		 * */
		void	set_name(const char* name);

		/**
		 * get thread pool name
		 * */
		const char* name() { return m_name; }

		/**
		 * get running state
		 **/
		bool	working() { return status(ST_work); }

	protected:
		/**
		 * insert new thread
		 **/
		void	insert_thread(Thread* thread);

		/**
		 * change to start status
		 **/
		int		change_start();

		/**
		 * fetch next task and run
		 **/
		bool	next(Thread* thread);

		/**
		 * thread wait
		 **/
		void	thread_wait(Thread* thread);

		/**
		 * do task work
		 **/
		void    task_work(Task* task, Thread* thread) {
			m_taskm->work(task, thread);
			cycle_task(task);
			at_inc64(m_done);
		}

		/**
		 * get next task
		 **/
		Task*	next_task() {
			task_deque_t& list = m_prior.empty() ?
				m_tasks : m_prior;
			Task* task = *list.begin();
			list.pop_front();
			return task;
		}

		/**
		 * cycle current task, mayb error happen
		 **/
		void	cycle_task(Task* task, int eno = 0) {
			m_taskm->cycle(task, eno);
		}

		/**
		 * clear all task
		 **/
		void	clear(int error = -1);

		/**
		 * wait all request done
		 **/
		void	wait_done();

		/**
		 * wait all task complete
		 **/
		int		cancel(bool wait_curr);

		/**
		 * decrease current count
		 **/
		void	dec_count(int64_t size);

		/**
		 * check if we just become to pause
		 **/
		void    check_pause();

		/**
		 * check if task is empty
		 **/
		bool	empty() { return m_prior.empty() && m_tasks.empty(); }

		/**
		 * check current status
		 **/
		bool	status(int st) { return m_status == st; }

		/**
		 * get current status
		 **/
		int		status() { return m_status; }

		/**
		 * check if working status
		 **/
		bool	running() { return status(ST_work) || status(ST_pause); }

	protected:
		typedef std::vector<Thread*> thread_queue_t;
		typedef std::deque<Task*> task_deque_t;

		/** mutex for lock */
	    Mutex 		m_mutex = {"thread pool", true};
	    /** wakeup condition */
	    Cond 		m_cond;
	    /** thread pool name */
	    char 		m_name[32] = {0};
	    /** current task count */
	    int64_t		m_curr = {0};
	    /** done count */
	    int64_t		m_done = {0};
		/** thread pool stop flag */
	    int			m_status = {ST_null};
	    /** thread container */
	    thread_queue_t	m_threads;
	    /** task manager */
	    TaskManage*  m_taskm = {NULL};
	    /** priority deque */
	    task_deque_t m_prior;
	    /** task deque */
	    task_deque_t m_tasks;
	};

	/**
	 * task manage
	 **/
	template <class TaskType = ThreadPool::Task>
	class TypeTaskManage : public ThreadPool::TaskManage
	{
	public:
		TypeTaskManage() {}

	public:
		/**
		 * malloc new task
		 **/
		virtual ThreadPool::Task* malloc(void* context) { return new TaskType(context); }

		/**
		 * call by thread pool, when thread pool stop or deny task; should dec task refer,
		 * if error happen, task will call with error
		 **/
		virtual void cycle(ThreadPool::Task* task, int eno = 0) {
			/** report task error */
			if (eno != 0) {
			}
			if (task->dec()) {
				delete (TaskType*)task;
			}
		}
	};
}

#if COMMON_SPACE
	using common::ThreadPool;
	using common::TypeTaskManage;
#endif
