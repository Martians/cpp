
#define LOG_CODE 	0

#include <string>
#include <deque>
#include <unistd.h>
#include <cstring>
#include <signal.h>

#include "Common/Common.hpp"
#include "Common/Atomic.hpp"
#include "Common/LogHelper.hpp"
#include "Common/ThreadInfo.hpp"
#include "Common/ThreadPool.hpp"

namespace common {

void
CommonThread::set_name(const char* name, int type)
{
	if (strlen(name) > 0) {
		assert(strlen(name) < sizeof(m_name));
		memcpy(m_name, name, strlen(name));
		m_type = type;

	} else if (strlen(m_name) > 0) {
        set_thread(m_name, m_type);
	}
}

int
CommonThread::create(bool detach, thread_handle_t handle, void* arg)
{
	//_num_threads.inc();
#if 0
	size_t stack_size;
	pthread_attr_getstacksize(&attr, &stack_size);
	stack_size *= 2;
	pthread_attr_setstacksize(&attr, stack_size);
#endif

	pthread_attr_t attr;
	memset(&attr, 0, sizeof(pthread_attr_t));
	pthread_attr_init(&attr);
	if (detach) {
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	}

	int r = pthread_create(&m_thread_id, &attr,
			handle ? handle : _entry_func,
			arg ? arg : (void*) this);
	//generic_dout(10) << "thread " << thread_id << " start" << dendl;
	pthread_attr_destroy(&attr);
	return r;
}

int
CommonThread::join(void **prval)
{
	if (m_thread_id == 0) {
		//generic_derr(0) << "WARNING: join on thread that was never started" << dendl;
		assert(0);
		return -EINVAL; // never started.
	}

	int status = pthread_join(m_thread_id, prval);
	if (status != 0) {
		switch (status) {
		case -EINVAL:
			//generic_derr(0) << "thread " << thread_id << " join status = EINVAL" << dendl;
			break;
		case -ESRCH:
			//generic_derr(0) << "thread " << thread_id << " join status = ESRCH" << dendl;
			assert(0);
			break;
		case -EDEADLK:
			//generic_derr(0) << "thread " << thread_id << " join status = EDEADLK" << dendl;
			break;
		default:
			;
			;
			//generic_derr(0) << "thread " << thread_id << " join status = " << status << dendl;
		}
		assert(0); // none of these should happen.
	}
	//generic_dout(10) << "thread " << thread_id << " stop" << dendl;
	m_thread_id = 0;
	return status;
}

int
CommonThread::detach()
{
	if (m_thread_id == 0) {
		return -EINVAL;
	}
	int status = pthread_detach(m_thread_id);
	if (status != 0) {
		switch (status) {
		case EINVAL:
			break;
		case ESRCH:
			break;
		default:
			break;
		}
	}
	return status;
}

int
CommonThread::kill(int signal)
{
	if (m_thread_id) {
		return pthread_kill(m_thread_id, signal);
	} else {
		return -EINVAL;
	}
}

void
ThreadPool::Thread::schedule(bool force)
{
	if (m_timer.wait() <= 0) {
		#if TIME_RECORD
		m_times.next("schedule");
		if (m_times.bomb("thread pool check task ")) {
			m_times.next();
		}
		#endif
		return;
	}

	bool trigger = force;
	if (!force) {
		if (m_cycle.check(false) && m_timer.expired()) {
			trigger = true;

		} else {
			debug("thread schedule, thread " << index() << ", ignore count "
				<< m_cycle.m_count << ", time rest " << m_timer.rest(false));
		}
	}

	if (trigger) {
		debug("thread schedule, thread " << index() << ", do schedule work, cycle " << m_cycle.m_count << ", force: " << force);
		handle();

		m_timer.reset();
		m_cycle.reset();
	}
}

int
ThreadPool::change_start()
{
	if (status(ST_pause)) {
		m_status = ST_work;
		trace("thread pool start, restart from pause");
		return 0;

	} else if (status(ST_work)) {
		trace("thread pool start, already start");
		return -1;
	}
	/** status == ST_null or status == ST_stop */
	m_status = ST_work;
	return 1;
}

int
ThreadPool::stop(bool wait_pend)
{
	if (wait_pend) {
		wait_done();
	}
	do {
		Mutex::Locker lock(m_mutex);
		/** multi-thread stop, only one wait for stop */
		if (status(ST_stop)) {
			debug("thread pool stop, already stop, ignore");
			return -1;
		}
		m_status = ST_stop;
		m_cond.signal_all();
		debug("thread pool stop, notify all");
	} while (0);

	cancel(true);

	for (auto& thread : m_threads) {
		thread->join();
		debug("thread pool stop, thread " << thread->index() << " join");
		delete thread;
	}
	m_threads.clear();
	debug("thread pool stop, done " << this->done());
	return 0;
}

int
ThreadPool::pause(bool wait_curr)
{
	do {
		Mutex::Locker lock(m_mutex);
		/** never recv new task */
		if (!status(ST_work)) {
			debug("thread pool pause, but status already "
				<< (status() == ST_pause ? "pause" : "stop"));
			return -1;
		}
		m_status = ST_pause;
		debug("thread pool pause, cancel pending task " << m_prior.size() + m_tasks.size()
			<< ", will deny new ones");
	} while (0);

	cancel(wait_curr);
	return 0;
}

int
ThreadPool::add(Task* task, bool front)
{
	Mutex::Locker lock(m_mutex);
	if (!working()) {
		cycle_task(task, ECANCELED);
		trace("add task, cancel it, thread pool already " << (status() == ST_pause ? "pause" : "stop"));
		return -1;
	}

	if (front) {
		m_tasks.push_front(task);
	} else {
		m_tasks.push_back(task);
	}
	atomic_inc64(&m_curr);
	m_cond.signal_one();

	//task->m_time = ctime_now();
	return 0;
}

/**
 * put task back to queue
 **/
int
ThreadPool::add_prior(Task* task, bool front)
{
	Mutex::Locker lock(m_mutex);
	if (!working()) {
		cycle_task(task, ECANCELED);
		trace("add prior, cancel it, thread pool already " << (status() == ST_pause ? "pause" : "stop"));
		return -1;
	}

	if (front) {
		m_prior.push_front(task);
	} else {
		m_prior.push_back(task);
	}
	atomic_inc64(&m_curr);
	m_cond.signal_one();
	return 0;
}

void
ThreadPool::set_name(const char* name)
{
	assert(strlen(name) <= sizeof(m_name));
	memcpy(m_name, name, strlen(name));

	if (Testing()) {
		if (strlen(m_name) != 0) {
			reset_thread(m_name);
		}
	}
}

void
ThreadPool::insert_thread(Thread* thread)
{
	thread->set_name(name(), ThreadInfo::TT_pool);
	m_threads.push_back(thread);
}

bool
ThreadPool::next(Thread* thread)
{
	#if TIME_RECORD
		thread->m_times.reset(true, 5000);
		thread->m_times.next("wait last");
	#endif
	Task* task = NULL;
	{
		Mutex::Locker lock(m_mutex);
		while (empty() && running()) {
			thread_wait(thread);
		}

		#if TIME_RECORD
			thread->m_times.next("wait sleep");
		#endif
		if (!running()) {
			debug("next task, thread " << thread->index() << " wakeup but already stop, exit");
			return false;

		} else if (!(task = next_task())) {
			debug("next task, thread " << thread->index() << " wakeup but no task");
			return true;
		}
		//assert(task->refer() > 0);
		debug("next task, thread " << thread->index() << " fetch, total " << count());
	}
	#if TIME_RECORD
		thread->m_times.next("fetch task");
	#endif
	//log_info("wait time " << string_timer(ctime_now() - task->m_time));

	task_work(task, thread);
	dec_count(1);

	#if TIME_RECORD
		thread->m_times.next("work task", 10000);
	#endif

	debug("next task, thread " << thread->index() << " complete, total " << count() << " run " << run_count());
	return true;
}

void
ThreadPool::thread_wait(Thread* thread)
{
	int wait = 0;
	/** no need wait */
	if ((wait = thread->wait_time()) == -1) {
		m_cond.wait(m_mutex);

	} else {
		debug("thread wait, thread " << thread->index() << " time rest " << wait);

		if (wait == 0 || m_cond.wait_interval(m_mutex, wait) == ETIMEDOUT) {
			debug("thread wait, thread " << thread->index()
				<< " wakeup for schedule, set time " << wait << " actual " << thread->timer().elapse());
			m_mutex.unlock();
			/** wakeup for timeout */
			thread->schedule(true);
			m_mutex.lock();
		}
	}
}

void
ThreadPool::dec_count(int64_t size)
{
	if (atomic_add64(&m_curr, -size) == size) {
		check_pause();
	}
}

void
ThreadPool::clear(int error)
{
	Mutex::Locker lock(m_mutex);

	size_t size = m_prior.size() + m_tasks.size();
	for (auto& task : m_prior) {
		cycle_task(task, error);
	}
	m_prior.clear();

	for (auto& task : m_tasks) {
		cycle_task(task, error);
	}
	m_tasks.clear();

	dec_count((int64_t)size);
	trace("clear task, set as error " << error << ", remain " << count());
}

int
ThreadPool::cancel(bool wait_curr)
{
	clear(ECANCELED);

	if (wait_curr) {
		debug("thread pool cancel, try to wait");
		wait_done();

		debug("thread pool cancel, wait done");
	} else {
		debug("thread pool cancel, no wait, remain " << count());
	}
	return 0;
}

void
ThreadPool::wait_done()
{
	while (count() > 0) {
		usleep(10 * c_time_level[0]);
	}
}

void
ThreadPool::check_pause()
{
	if (status(ST_pause)) {
		trace("thread pool check pause, last thread paused");
		for (auto& thread : m_threads) {
			thread->pause();
		}
	}
}

void main_loop()
{
	while (1) {
		usleep(c_time_level[1]);
	}
}

};

#if COMMON_TEST
#include "Common/Display.hpp"
//#include <functional>

namespace common {
namespace tester {

	struct ThreadParam
	{
		ThreadParam() {
			trace("struct, address " << value);
		}

		ThreadParam(const ThreadParam& v) {
			trace("copy struct, address " << value);
			//assert(0);
		}

		const ThreadParam& operator = (const ThreadParam& v) {
			trace("operator = , address " << value);
			assert(0);
			return *this;
		}
		void* value = {this};
	};

	class ThreadTest : public common::ThreadPool::Thread
	{
	public:
		ThreadTest(common::ThreadPool* pool, int index)
			: common::ThreadPool::Thread(pool, index)
		{
			timer(50);
		}

#if 0
		template<typename...T>
		void	set(std::tuple<T...> param) {
			//ThreadParam tp; int a;
			//std::tie(tp, a) = param;
			trace("address " << std::get<0>(param).value << ", int " << std::get<1>(param));
		}
#else
		void set(const ThreadParam& param, int value) {
			trace("address " << param.value << ", int " << value);
		}
#endif

		virtual void handle() {
			trace("handle, elapse " << string_record(m_record));
		}
	protected:
		TimeRecord m_record;
	};

	class NewTask : public common::ThreadPool::Task
	{
	public:
		NewTask(void* param) {}
		virtual bool operator ()() {
			usleep(5000);
			return true;
		}
	};

	void
	thread_pool_test()
	{
		ThreadParam s_param;
		common::ThreadPool pool("test", new common::TypeTaskManage<NewTask>());
		pool.start<ThreadTest>(100, s_param, 2);

		for (int i = 0; i < 10000; i++) {
			pool.add((void*)NULL);
		}
		pool.stop(true);
	}
}
}
#endif

