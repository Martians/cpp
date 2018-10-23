
#include <cstring>

#include "Common/Display.hpp"
#include "Common/ThreadInfo.hpp"

#include "Perform/Debug.hpp"
#include "Perform/Timer.hpp"

namespace common {
namespace tester {
/** thread vector instance */
ThreadVector ThreadVector::s_pool;

/**
 * regist time warn
 **/
const ctime_t c_time_warn_level[] = {
	5  * c_time_level[0],
	50 * c_time_level[0],
	2  * c_time_level[1],
	4  * c_time_level[1],
	10 * c_time_level[1],
};

bool
StadgeTimer::bomb(const char* prefix)
{
	for (int index = 0; index < m_index; index++) {
		ctime_t bomb = m_item[m_index].bomb == 0 ? m_bomb :
			m_item[m_index].bomb;
		if (time(index) >= bomb) {
			if (prefix) {
				dump(prefix);
			}
			return true;
		}
	}
	return false;
}

std::string&
StadgeTimer::string(std::string& value)
{
	init();

	int count = 0;
	Item* item;
	const char* sep = "\n";
	while ((item = get()) != NULL) {
		append(value, "%s \t<%d \t %10s %s", sep, count++,
			string_timer(time(item)).c_str(), item->name);
		sep = "\n";
	}
	return value;
}

void
StadgeTimer::dump(const char* prefix)
{
	std::string value;
	log_info(prefix << string(value));
}

bool
need_warn(ctime_t time)
{
	return time >= c_time_warn_level[0];
}

bool
time_bombed()
{
	ctime_t time = common::time_last(true);
	if (time > 0 && need_warn(time)) {
		return true;
	}
	return false;
}

const char*
time_warn_string(ctime_t last)
{
	if (last > c_time_warn_level[0]) {

		if (last > c_time_warn_level[4]) {
			return " ========== delay - 4 DISASTER ==========\n\n\n\n\n"
				   " ========== delay - 4 DISASTER ==========";
		} else if (last > c_time_warn_level[3]) {
			return " ========== delay - 3 FATAL ==========";

		} else if (last > c_time_warn_level[2]) {
			return " ========== delay - 2 WARN ==========";

		} else if (last > c_time_warn_level[1]) {
			return " ========== delay - 1 INFO ==========";

		} else {
			return " ========== delay - 0 ==========";
		}
	}
	return "";
}

std::string
time_using(bool mute_warn)
{
	ctime_t time = common::time_last(false);
	if (time > 0) {

		char data[256] = {0};
		snprintf(data, 256, "  - %.3f ms", (float)time/c_time_level[0]);

		if (!mute_warn && need_warn(time)) {
			strcat(data, time_warn_string(time));
		}
		return data;
	}
	return "";
}
}

class TimeRecord::OutputScope {
public:
	OutputScope(TimeRecord& _record, const char* _name)
		: record(_record), name(_name)
	{
	}
	virtual ~OutputScope() {
		log_debug(name << " using " << string_record(record));
	}

protected:
	TimeRecord& record;
	const std::string& name;
};
}

#if COMMON_TEST
#include <unistd.h>

namespace common {
namespace tester {

	void
	__time_thread(int type = common::ThreadInfo::TT_normal, const char* name = "", ctime_t sleep = 0)
	{
        common::set_thread(name, type);
		usleep(sleep);
		time_debug_using("thread " << pthread_self());
	}

	void
	time_thread_test()
	{
		single(__time_thread, common::ThreadInfo::TT_main, "main", 0);
		single(__time_thread, common::ThreadInfo::TT_pool, "pool", 0);
		single(__time_thread, common::ThreadInfo::TT_pool, "pool", 0);
		single(__time_thread, common::ThreadInfo::TT_pool, "pool", 100);
		single(__time_thread, common::ThreadInfo::TT_normal, "", 30);
		single(__time_thread, common::ThreadInfo::TT_normal, "", 200);
		single(__time_thread, common::ThreadInfo::TT_normal, "", 0);

		batchs(10, __time_thread, common::ThreadInfo::TT_pool, "pool", 0);
	}
}
}
#endif
