
#include <memory>
#include <cstring>

#include "Common/Define.hpp"
#include "Common/Mutex.hpp"
#include "Common/ThreadInfo.hpp"
#include "Common/Container.hpp"
#include "Common/CodeHelper.hpp"

namespace common {

ThreadInfo::ThreadInfo(int index, int type, const char* name, int64_t pid)
	: m_index(index), m_type(type), m_pid(pid)
{
	memcpy(m_name, name, strlen(name));
}

/**
 * used for singleton mutex, in case multi initialize
 **/
class ThreadContext
{
public:
	ThreadContext() : mutex("thread mutex", true) {}

	/**
	 * reset regist item
	 **/
	void	reset(const std::string& name) {
		if (name.length() == 0) {
			exist.clear();
			index.clear();
		} else {
			exist.del(name);
			index.del(name);
		}
	}

public:
	Mutex	mutex;
	/** thread index */
	int		thidx = {0};
	TypeSet<std::string> exist;
	TypeMap<std::string, int> index;
};

//ThreadMutex& s_thread_context = *Singleton<ThreadMutex>::get();
SINGLETON(ThreadContext, thread_contex);

/** define current thread */
//extern thread_local std::shared_ptr<ThreadInfo> th_curr;
thread_local std::shared_ptr<ThreadInfo> th_curr;

ThreadInfo* thread_info()
{
	return th_curr.get();
}

void
reset_thread(const char* name)
{
	thread_contex().reset(name);
}

int
pool_thread_index(const char* name)
{
	int* index = thread_contex().index.get(name);
	if (index) {
		(*index)++;
		return *index;

	} else {
		thread_contex().index.add(name, 0);
	}
	return 0;
}

void
set_thread(const char* name, int type)
{
	if (th_curr) {
		return;
	}
	Mutex::Locker lock(thread_contex().mutex);

	char data[64] = {0};
	if (type == ThreadInfo::TT_main) {
		memcpy(data, name, strlen(name));
		success(thread_contex().exist.add(name));

	} else if (type == ThreadInfo::TT_pool) {
		snprintf(data, 64, "%s-%02d", name, pool_thread_index(name));

	} else {
		type = ThreadInfo::TT_normal;
		static int s_normal_index = 0;
		snprintf(data, 64, "%04d", s_normal_index++);
	}

	assert(th_curr == NULL);
	th_curr = std::make_shared<ThreadInfo>(
		++thread_contex().thidx, type, data, (int64_t)pthread_self());
}

}


