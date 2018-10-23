
#pragma once

#include <string>
#include <cstdarg>

#if COMMON_TEST
#include <sstream>
#endif

#include "Common/Const.hpp"
#include "Common/Define.hpp"
#include "Common/Common.hpp"
#include "Common/Time.hpp"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#define u64 PRIu64
#define i64 PRId64

namespace common {
	/**
	 * trigger fault output string
	 **/
	#define	fault_string(fmt, arg...)	fault_format(fmt, "", ##arg)

	/**
	 * trigger fault and output string with system error
	 * */
	#define	fault_syserr(fmt, arg...)	fault_format(fmt, (", " + syserr()).c_str(), ##arg)

	/**
	 * assert format
	 **/
	#define assert_string(value, fmt, arg...)	do { if (!(value)) { fault_string(fmt " ["#value"]" , ##arg); } } while (0)

	/**
	 * assert format with system error
	 **/
	#define assert_syserr(value, fmt, arg...)	do { if (!(value)) { fault_syserr(fmt " ["#value"]", ##arg); } } while (0)

	/**
	 * @brief printf c++ style info
	 */
	#define prints(s)		::std::cout << s << ::std::endl

	/**
	 * number to string
	 **/
	template<class T>
	::std::string n2s(T v) {
		char data[64] = {0};
		if (v < 0) {
			snprintf(data, sizeof(data), "%" i64, (int64_t)v);
		} else {
			snprintf(data, sizeof(data), "%" u64, (uint64_t)v);
		}
		return data;
	}
	/**
	 * string to number
	 **/
	template<class T>
	T s2n(const ::std::string& v) {
		return (T)atoll(v.c_str());
	}

	#if COMMON_TEST
	/**
	 * number to string
	 **/
	template<class T>
	::std::string n2s_s(T v) {
		::std::stringstream ss;
		///< in case scientific notation
		ss.imbue(::std::locale("C"));
		ss << v;
		return ss.str();
	}
	/**
	 * string to number
	 **/
	template<class T>
	T s2n_s(const ::std::string& s) {
		if (s.length() == 0) {
			return 0;
		}
		::std::stringstream ss;
		T v;
		ss << s;
		ss >> v;
		return v;
	}
	#endif

	/**
	 * get integer unit
	 **/
	int		integer_unit(const std::string& str);

	/**
	 * get int unit
	 **/
	template<class Type>
	Type	string_integer(const std::string& str) {
		int unit = integer_unit(str);
		return s2n<Type>(str)* unit;
	}

	/**
	 * format sprintf
	 **/
	#define FORMAT_SIZE(size, fmt) 	\
		char data[size];			\
		va_list args;				\
		va_start(args, fmt);		\
		vsnprintf(data, sizeof(data), fmt, args)
	#define FORMAT(fmt)	FORMAT_SIZE(c_length_64K, fmt)

	/**
	 * get formated system error string
	 * */
	::std::string syserr(int err = 0, bool format = true);

	/**
	 * get elapse time, like [00:01:56.56781]
	 * */
	::std::string string_elapse(const ctime_t& last);

	/**
	 * get timer using, like 0, 100 ms, 10.567 ms, 10.456 s
	 **/
	::std::string string_timer(uint64_t last, bool simple = false, bool us = true);

	/**
	 * get timer using, like 0, 100 ms, 10.567 ms, 10.456 s, will change timer
	 **/
	inline ::std::string string_record(TimeRecord& time, bool simple = false, bool us = true) {
		return string_timer(time.check(), simple, us);
	}

	/**
	 * get timer using, like 0, 100 ms, 10.567 ms, 10.456 s, will not change timer
	 **/
	inline ::std::string string_timer(TimeRecord& time, bool simple = false, bool us = true) {
		return string_timer(time.last(), simple, us);
	}

	/**
	 * get count string, like 1093, 89w, 89w
	 **/
	::std::string string_count(uint64_t count, bool convert = true);

	/**
	 * get size string, like 893, 10 M, 20M, 10 K
	 **/
	::std::string string_size(uint64_t size, bool convert = true);

	/**
	 * get iops string, like 1089
	 * */
	::std::string string_iops(uint64_t iops, ctime_t last);

	/**
	 * get latancy, like 35.893 ms
	 * */
	::std::string string_latancy(uint64_t iops, ctime_t last);

	/**
	 * get speed string, like 89.789 M/s, 984 B/s
	 **/
	::std::string string_speed(uint64_t size, ctime_t last);

	/**
	 * get percent, like 89.1%
	 * */
	::std::string string_percent(uint64_t size, uint64_t total);

	/**
	 * get current data, like 2015-11-09
	 * */
	::std::string	string_date();

	/**
	 * get current time, like 12:25:40.16459 or 12:25:40
	 * */
	::std::string	string_time(bool usec = false);

	/**
	 * @brief format string and return string
	 */
	::std::string	format(const char* fmt, ...);

	/**
	 * @brief format string and return string
	 * @param s the dst string
	 * @return formated string s
	 */
	::std::string& format(::std::string& str, const char* fmt, ...);

	/**
	 * @brief append string and return string
	 * @param s the dst string
	 * @return formated string s
	 */
	::std::string& append(::std::string& str, const char* fmt, ...);

	/**
	 * dump if exist
	 **/
	::std::string  dump_helper(bool exist, const ::std::string& value);

	/**
	 * string helper
	 **/
	class String : std::string
	{
	public:
		typedef std::string base;
		using base::base;
	};

	namespace helper {
		/**
		 * @brief trace data content
		 */
		::std::string data_trace(const void* data, uint32_t len);

		/**
		 * dump display reset split
		 **/
		void	dump_reset(const ::std::string& split = " ");

		/**
		 * dump value if exist
		 **/
		::std::string	dump_exist(bool exist, const ::std::string& value);

		/**
		 * dump last if exist something, with prefix and suffix
		 **/
		::std::string	dump_last(const ::std::string& prefix = "", const ::std::string& suffix = "");
	}
}

