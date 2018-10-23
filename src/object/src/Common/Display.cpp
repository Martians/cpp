
#include <cassert>
#include <stdio.h>
#include <cstring>

#include "Common/Const.hpp"
#include "Common/Display.hpp"

namespace common {

int
integer_unit(const std::string& str)
{
	int unit = 1;
	if (str.length() >= 2) {
		std::string::size_type pos;;
		if ((pos = str.find_first_of("Kk"))
			!= std::string::npos)
		{
			unit = c_length_1K;
		} else if ((pos = str.find_first_of("Mm"))
			!= std::string::npos)
		{
			unit = c_length_1M;
		} else if ((pos = str.find_first_of("Gg"))
			!= std::string::npos)
		{
			unit = c_length_1G;
		}
	}
	return unit;
}

::std::string
syserr(int err, bool format)
{
	if (err == 0) err = errno;
	else if (err < 0) err = -err;

    if (format) {
        char data[256];
        snprintf(data, sizeof(data), "sys err: %d - %s",
                 err, strerror(err));
        return data;
    } else {
        return strerror(err);
    }
}

::std::string
string_elapse(const ctime_t& last)
{
	char data[64];
	ctime_t st = last / c_time_level[1];
	ctime_t tp = st % 3600;

	snprintf(data, sizeof(data), "[%02d:%02d:%02d.%06ld]",
		(int)st/3600, (int)tp/60, (int)tp%60, last % c_time_level[1]);
	return data;
}

::std::string
string_timer(uint64_t last, bool simple, bool us)
{
	char data[64];

	const char* level = "ms";
	if (last > (uint64_t)c_time_level[1]) {
		last = last / c_time_level[0];

		simple = false;
		level = "s";
	}

	if (last == 0) {
		return "0";

	} else if (us) {
		if (simple) {
			snprintf(data, 64, "%lld %s", (long_int)last/c_time_level[0], level);

		} else {
			snprintf(data, 64, "%.3f %s", (float)last/c_time_level[0], level);
		}

	} else {
		snprintf(data, 64, "%lld %s", (long_int)last, level);
	}
	return data;
}

::std::string
string_count(uint64_t count, bool convert)
{
	char data[64];
	if (convert) {
		if (count >= 100000000) {
			snprintf(data, sizeof(data), "%3.2f E", (float)count/100000000);
		} else if (count >= 10000) {
			snprintf(data, sizeof(data), "%3.1f W", (float)count/10000);
		} else {
			snprintf(data, sizeof(data), "%3lld",   (long long int)count);
		}
	} else {
		if (count >= 100000000)	{
			snprintf(data, sizeof(data), "%lldE",	(long long int)count/100000000);
		} else if (count >= 10000) {
			snprintf(data, sizeof(data), "%lldW", 	(long long int)count/10000);
		} else {
			snprintf(data, sizeof(data), "%lld", 	(long long int)count);
		}
	}
	return data;
}

::std::string
string_size(uint64_t size, bool convert)
{
	char data[64];
	if (convert) {
		if (size >= c_length_1E) {
			snprintf(data, sizeof(data), "%.3f T",(float)size/c_length_1E);
		} else if (size >= c_length_1G) {
			snprintf(data, sizeof(data), "%.3f G",(float)size/c_length_1G);
		} else if (size >= c_length_1M) {
			snprintf(data, sizeof(data), "%.2f M",(float)size/c_length_1M);
		} else if (size >= c_length_1K) {
			snprintf(data, sizeof(data), "%.2f K",(float)size/c_length_1K);
		} else {
			snprintf(data, sizeof(data), "%d",   (uint32_t)size);
		}
	} else {
		if (size >= c_length_1E) {
			snprintf(data, sizeof(data), "%lldT", (long long int)size/c_length_1E);
		} else if (size >= c_length_1G)	{
			snprintf(data, sizeof(data), "%lldG", (long long int)size/c_length_1G);
		} else if (size >= c_length_1M) {
			snprintf(data, sizeof(data), "%lldM", (long long int)size/c_length_1M);
		} else if (size >= c_length_1K) {
			snprintf(data, sizeof(data), "%lldK", (long long int)size/c_length_1K);
		} else {
			snprintf(data, sizeof(data), "%lld",  (long long int)size);
		}
	}
	return data;
}

::std::string
string_iops(uint64_t iops, ctime_t last)
{
	last = ::std::max(last, (ctime_t)1);

	if (iops == 0) {
		return "0";
	}
	char data[64];
	snprintf(data, sizeof(data), "%lld", (long long int)(iops * c_time_level[1] / last));
	return data;
}

::std::string
string_latancy(uint64_t iops, ctime_t last)
{
	iops = ::std::max(iops, (uint64_t)1);

	char data[64];
	snprintf(data, sizeof(data), "%2.3f ms", (float)last / c_time_level[0] / iops);
	return data;
}

::std::string
string_speed(uint64_t size, ctime_t last)
{
	last = ::std::max(last, (ctime_t)1);
	float speed = (float)size * c_time_level[1] / last;

	char data[64];
	if (speed >= c_length_1M) {
		snprintf(data, sizeof(data), "%3.2f M/s", speed/c_length_1M);
	} else if (speed >= c_length_1K) {
		snprintf(data, sizeof(data), "%3.1f K/s", speed/c_length_1K);
	} else{
		snprintf(data, sizeof(data), "%3.1f B/s", speed);
	}
	return data;
}

::std::string
string_percent(uint64_t size, uint64_t total)
{
	total = ::std::max(total, size);

	if (size == 0) {
		return "0";
	}
	char data[64];
	snprintf(data, sizeof(data), "%2.1f%%", (float)size * 100 / total);
	return data;
}

::std::string
string_date()
{
	/** format time */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	::time_t lt = tv.tv_sec;
	struct tm* crttime = localtime(&lt);
	assert(crttime != NULL);

	char data[64];
	snprintf(data, 64, "%02d-%02d-%02d",
			crttime->tm_mon + 1, crttime->tm_mday, crttime->tm_year + 1900 - 2000);
	return data;
}

::std::string
string_time(bool usec)
{
	/** format time */
	struct timeval tv;
	gettimeofday(&tv, NULL);
	::time_t lt = tv.tv_sec;
	struct tm* crttime = localtime(&lt);
	assert(crttime != NULL);

	char data[64];
	if (usec) {
		snprintf(data, 64, "%02d:%02d:%02d.%06d",
				crttime->tm_hour, crttime->tm_min, crttime->tm_sec, (int)tv.tv_usec);
	} else {
		snprintf(data, 64, "%02d:%02d:%02d",
				crttime->tm_hour, crttime->tm_min, crttime->tm_sec);
	}
	return data;
}

template<class...Type>
::std::string format_type(Type...args) {
    char data[4096];
	snprintf(data, sizeof(data), args...);
    return data;
}

::std::string
format(const char * fmt,...)
{
	FORMAT(fmt);
	return data;
}

::std::string&
format(::std::string& str, const char * fmt,...)
{
	FORMAT(fmt);
	str = data;
	return str;
}

::std::string&
append(::std::string& str, const char * fmt,...)
{
	FORMAT(fmt);
	str += data;
	return str;
}
}

#include "Common/CodeHelper.hpp"

namespace common {
namespace helper {

	::std::string
	data_trace(const void* buffer, uint32_t len)
	{
		char data[4096];
		char temp[1024];
		::std::string str;

		for (uint32_t index = 0; index < len; index++) {
			if (index % 8 == 0) {
				snprintf(temp, sizeof(temp), "<%-3d ", index);
				if (index != 0) {
					str += "\n";
				}
			} else {
				temp[0] = 0;
			}
			snprintf(data, sizeof(data), "%s %2x ", temp, *((char*)buffer + index));

			str += data;
		}
		return str;
	}

	struct StringDumpHelper
	{
		int 	index = 0;
		::std::string dump;
		::std::string split = " ";
	};
	SINGLETON(StringDumpHelper, DumpHelper)

	void
	dump_reset(const ::std::string& split)
	{
		StringDumpHelper& helper = DumpHelper();
		helper.index = 0;
		helper.dump  = "";
		helper.split = split;
	}

	::std::string
	dump_exist(bool exist, const ::std::string& value)
	{
		if (!exist) {
			return "";
		}

		StringDumpHelper& helper = DumpHelper();
		helper.dump = value + (helper.index > 0 ? helper.split : "") + helper.dump;
		helper.index++;
		return "";
	}

	/**
	 * usage:
	 * 	dump_reset("");
	 * 	ss << dump_last("[", "]")
	 * 	   << dump_exist(A_exist, "A")
	 * 	   << dump_exist(B_exist, "B");
	 **/
	::std::string
	dump_last(const ::std::string& prefix, const ::std::string& suffix)
	{
		StringDumpHelper& helper = DumpHelper();
		if (helper.index == 0) {
			return helper.dump;
		}
		helper.dump = prefix + helper.dump + suffix;

		::std::string temp = helper.dump;
		dump_reset(helper.split);
		return temp;
	}
}
}

#if COMMON_TEST
	#include "Perform/Timer.hpp"
	#include "Perform/Debug.hpp"

namespace common {
namespace tester {
	int64_t __convert(int type, bool num) {
		int64_t temp = 0;
		::std::string c_str;
		int64_t c_int;
		const char* str = "892340973656";
		int64_t val = 892340973656;
		for (int i = 0; i < 1000000; i++) {
			if (type == 1) {
				if (num) {
					c_int = s2n_s<int64_t>(str);
					assert(c_int == val);
					temp += c_int;
				} else {
					c_str = n2s_s(val);
					assert(c_str == str);
				}
			} else {
				if (num) {
					c_int = s2n<int64_t>(str);
					assert(c_int == val);
					temp += c_int;
				} else {
					c_str = n2s(val);
					assert(c_str == str);
				}
			}
		}
		return temp;
	}

	void
	convert_test() {
		CREATE_TIMER;

		__convert(1, true);
		THREAD_TIMER("sstream s2n");

		__convert(1, false);
		THREAD_TIMER("sstream n2s");

		__convert(2, true);
		THREAD_TIMER("atoll s2n");

		__convert(2, false);
		THREAD_TIMER("atoll n2s");
	}
}
}
#endif

#if 0
int
ywb_hexdecodechar(char v)
{
	if (v >= '0' && v <= '9') {
		return v - '0';
	}
	if (v >= 'a' && v <= 'z') {
		return v - 'a' + 10;
	}
	if (v >= 'A' && v <= 'Z') {
		return v - 'A' + 10;
	}
	return -1;
}

int
ywb_hexdecodeint(const char *buf, int buflen, int l, ywb_uint64_t *v)
{
	if (NULL == v) {
		return -1;
	}
	*v = 0;
	if (buflen > 16) {
		buflen = 16;
	}
    int i = 0;
	for (i = 0; i < buflen; i ++) {
		int a = ywb_hexdecodechar(buf[i]);
		if (a < 0) {
			return YWB_EINVAL;
		}
		*v = ((*v) << 4) | ywb_hexdecodechar(buf[i]);
	}
	return 0;
}

int
ywb_hexencode(const ywb_byte_t *d, ywb_size_t d_len, char *buf, ywb_size_t buflen)
{
	if (buflen < 2 * d_len) {
		return YWB_EINVAL;
	}
    ywb_size_t i = 0;
	for (i = 0; i < d_len; i ++) {
		ywb_hexencodeint(1, d[i], buf + (i << 1), 2);
	}
	return 0;
}
#endif


