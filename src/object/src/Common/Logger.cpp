

#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <cstdarg>
#include <cassert>
#include <vector>
#include <algorithm>
#include <iostream>

#include "Common/Display.hpp"
#include "Common/Logger.hpp"
#include "Common/Atomic.hpp"
#include "Common/Mutex.hpp"
#include "Common/Util.hpp"
#include "Common/File.hpp"
#include "Common/String.hpp"
#include "Common/ThreadInfo.hpp"
#include "Advance/Barrier.hpp"

namespace common {
//Barrier& s_log_barrier = *Singleton<Barrier>::get();
static common::FastBarrier s_log_barrier;
Logging Logging::s_instance[c_log_index_max] = {};

const char* Logging::c_log_path = "/var/log";
const char* Logging::c_log_name = "output";
const char* c_log_suffix = ".log";

uint32_t default_log_handler(int logfd, LogLevel level, Logging::LogParam* param, const char* string);

const char*
level_name(int level)
{
	static const char* level_names[] = { "INVLAID", "TRACE", "DEBUG",
		"INFO ", "WARN", "ERROR", "FATAL", "CODE"};

	if (level < LOG_LEVEL_NULL || level > LOG_LEVEL_CODE) {
		level = LOG_LEVEL_NULL;
	}
	return level_names[level];
}

struct Logging::LogVec : public std::vector<int>
{
};

Logging::Logging()
	: m_handle(default_log_handler), m_logvec(new LogVec)
{
}

Logging::~Logging()
{
	close();

    common::reset(m_logvec);
}

void
Logging::close()
{
	if (m_logfd != c_invalid_handle) {
		::close(m_logfd);
		m_logfd = c_invalid_handle;
	}
	m_length = 0;
}

void
Logging::set_name(const std::string& name, const std::string& path, int index)
{
	Logging& logger = s_instance[index];
	/** only initialize once */
	assert_string(logger.m_index == c_log_index_invalid,
		"logger already initialized with name");
	logger.close();

	if (name.length() == 0) {
		std::stringstream ss;
		ss << index << "_" << logger.m_name;
		logger.m_name = ss.str();

	} else {
		logger.m_name = name;
	}

	logger.m_path  = path;
	/** set as initialized */
	logger.m_index = index;
}

void
Logging::set_limit(int64_t size, int count, int index)
{
	Logging& logger = s_instance[index];

	logger.m_limit.size  = size == 0 ? c_logging_size : size;
	logger.m_limit.count = count == 0 ? c_logging_count: count;
}

void
Logging::set_mode(int dest, int mode, const char* prefix, int index)
{
	Logging& logger = s_instance[index];

	logger.m_param.dest = (dest == 0) ? LD_file : dest;
	logger.m_param.mode = (mode == 0) ? LM_null : mode;
	logger.m_param.prefix = prefix;
}

void
Logging::initialize()
{
	if (initialized()) return ;
    m_initial = true;

	if (m_path.length() == 0) {
		m_path = common::split_path(common::module_path());
	}
	m_path = common::cut_end_slash(m_path);

	if (common::make_path(m_path) != 0) {
		fault_syserr("make path %s for Logging failed", m_path.c_str());
	}

	struct dirent storge, *entry;
	DIR *dir = opendir(m_path.c_str());
	if (dir == NULL) {
		fault_syserr("read Logging dir %s failed", m_path.c_str());
	}
	int index = 0;
	while (readdir_r(dir, &storge, &entry) == 0 && entry) {
		const std::string curr = entry->d_name;

		/** check file type */
		if (common::file_dir(m_path + "/" + entry->d_name)) {
			continue;

		/** check file name length and suffix */
		} else if (curr.length() < m_name.length() ||
			curr.length() < strlen(c_log_suffix) ||
			strncmp(curr.c_str(), m_name.c_str(), m_name.length()) != 0) {
			continue;
		}

		int beg = m_name.length();
		int end = curr.length() - strlen(c_log_suffix);
		std::string si = curr.substr(beg, end - beg);

		/** check file suffix */
		if (strncmp(curr.substr(end).c_str(), c_log_suffix, strlen(c_log_suffix)) != 0) {
			continue;

		} else if (!common::isdigit(si)) {
			continue;
		}

		index = si.length() == 0 ? 0 : s2n<int>(si);
		m_logvec->push_back(index);
	}
	closedir(dir);

	std::sort(m_logvec->begin(), m_logvec->end(), std::greater<int>());
}

std::string
Logging::get_path(int num)
{
    std::string log_name = m_name + c_log_suffix;
    std::string full = m_path.length() == 0 ? m_path : (m_path + "/");
	if (num == 0) {
		log_name = full + log_name;

	} else {
		log_name = common::format("%s%s%d%s", full.c_str(), m_name.c_str(), num, c_log_suffix);
	}
	return log_name;
}

void
Logging::roll()
{
	close();

	for (auto iter = m_logvec->begin(); iter != m_logvec->end();) {
		int index = (*iter);
		std::string _old = get_path(index);
		std::string _new = get_path(index + 1);

		if (!common::file_exist(_old)) {
			iter = m_logvec->erase(iter);
			continue;

		} else if ((int64_t)m_logvec->size() >= m_limit.count) {

			if (common::file_rm(_old) != 0) {
				fault_syserr("remove old Logging %s failed", _old.c_str());
			}
			iter = m_logvec->erase(iter);
			continue;

		} else if (common::file_move(_old, _new) != 0) {
			fault_syserr("move Logging %s to %s failed", _old.c_str(), _new.c_str());
			iter = m_logvec->erase(iter);
			continue;
		}

		(*iter)++;
		iter++;
	}
	m_logvec->push_back(0);
}

void
Logging::log_format(LogLevel level, const char* filename, int line, const char *fmt, ...)
{
    FORMAT_SIZE(c_log_max_size, fmt);

    format(level, filename, line, data);
}

void
Logging::format(LogLevel level, const char* string, bool lock)
{
	if (m_logfd == c_invalid_handle) {
        Barrier::Block block(s_log_barrier, lock);
		if (m_logfd == c_invalid_handle) {
		    initialize();
			m_logfd = ::open(get_path().c_str(), O_RDWR | O_CREAT | O_APPEND, 0);
			assert_syserr(m_logfd != c_invalid_handle, "log open");
			m_length = ::common::file_len(m_logfd);

			static const std::string s_log_uniq =
				common::format("new start =============== [%s] ===============", string_time(true).c_str());
			format(LOG_LEVEL_INFO, "", false);
			format(LOG_LEVEL_INFO, "", false);
			format(LOG_LEVEL_INFO, s_log_uniq.c_str(), false);
		}
	}
    
    {
        Barrier::Enter enter(s_log_barrier, lock);
        if (m_logfd == c_invalid_handle) {
            enter.unblock();
            assert(lock == true);
            format(level, string, lock);
            return;
        }
	    int64_t writen = m_handle(m_logfd, level, &m_param, string);
	    common::atomic_add64(&m_length, writen);
    }

	if (m_length > m_limit.size) {
        Barrier::Block block(s_log_barrier, lock);
		if (m_length > m_limit.size) {
			roll();
		}
	}
}

uint32_t
default_log_handler(int logfd, common::LogLevel level, Logging::LogParam* param, const char* string)
{
	char temp[Logging::c_log_max_size];
	char* data = temp;
	int length = 0;

	/** format time and common::LogLevel */
	{
		struct timeval tv;
		gettimeofday(&tv, NULL);
		::time_t lt = tv.tv_sec;
		struct tm* crttime = localtime(&lt);
		assert(crttime != NULL);

		data += snprintf(data, 64, "[%02d-%02d-%02d %02d:%02d:%02d.%06ld] %-5s - ",
			crttime->tm_mon + 1, crttime->tm_mday, crttime->tm_year + 1900 - 2000,
			crttime->tm_hour, crttime->tm_min, crttime->tm_sec, tv.tv_usec,
			level_name(level));
	}

	if (param->prefix) {
		data += snprintf(data, 64, "[%s] ", param->prefix);

	/** format thread name */
	} else {
		const char* thread = common::thread_name();
		if (thread) {
			data += snprintf(data, 64, "[%s] ", thread);
		}
	}

	/** format display string */
	{
		length = (int)strlen(string);
		/** check remain length, */
		if (Logging::c_log_max_size - (int)(data - temp) < length) {
			length = Logging::c_log_max_size - (int)(data - temp);
		}
		/** format log data */
		memcpy(data, string, length);
		data += length;
	}

	if (common::get_bit(param->mode, Logging::LM_line)) {
		data += snprintf(data, 64, " [%s:%d]", [param]() {
			const char* last = strrchr(param->filename, '/');
			return last ? last + 1 : param->filename; }(), param->line);
	}

	/** set ending */
	{
		const char* ending = "\n\0";
		memcpy(data, ending, strlen(ending));
		data += 1;

	    assert(logfd != c_invalid_handle);
		length = data - temp;
	}

	int ret = 0;
	if (common::get_bit(param->dest, Logging::LD_file)) {
		ret = write(logfd, temp, length);
		assert(ret == length);
	}

	if (common::get_bit(param->dest, Logging::LD_syslog)) {
		assert_later(Logging::LD_syslog);
	}

	if (common::get_bit(param->dest, Logging::LD_stdout)) {
		std::cout << temp << std::flush;
	}
	return length;
}
}

#if COMMON_TEST
#include <unistd.h>
#include "Perform/Debug.hpp"

namespace common {
namespace tester {

	void
	__write_log(int count)
	{
		while (count-- >= 0) {
			usleep(1);
			log_info("write Logging " << pthread_self());
		}
	}

	void
	logger_test() {
        common::Logging::set_limit(1024 * 10, 30);
		batchs(30, __write_log, 1000);

        thread_wait();
        common::Logging::set_limit();

       //Logging::set_mode(make_bit(Logging::LD_file, Logging::LD_stdout),
       //	make_bit(Logging::LM_line));
	}
}
}
#endif


