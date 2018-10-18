
#pragma once

#include <sstream>
#include "Common/Const.hpp"

namespace common {
	/**
	 * log level enum
	 * */
	enum LogLevel {
		LOG_LEVEL_NULL	  = 0,
		LOG_LEVEL_TRACE   = 1,
		LOG_LEVEL_DEBUG   = 2,
		LOG_LEVEL_INFO    = 3,
		LOG_LEVEL_WARN	  = 4,
		LOG_LEVEL_ERROR   = 5,
		LOG_LEVEL_FATAL   = 6,
		LOG_LEVEL_CODE    = 7,

		LOG_START = 0,
	};

	/**
	 * base logger
	 **/
	class Logging
	{
	public:
		Logging();
		virtual ~Logging();

	public:
		/**
		 * log output bits
		 **/
		enum LogDest {
			LD_null = 0,
			/** log to file */
			LD_file,
			/** log to sys */
			LD_syslog,
			/** log to std */
			LD_stdout,
		};

		/**
		 * log output bits
		 **/
		enum LogMode {
			LM_null = 0,
			/** current code position */
			LM_line,
		};

		/**
		 * log param
		 **/
		struct LogParam {
			/** current output string */
			const char* string 	= {NULL};
			/** current log prefix */
			const char* prefix 	= {NULL};
			/** current file name */
			const char* filename = {NULL};
			/** current file line */
			int 	line = {0};
			/** output destination */
			int		dest = {LD_file};
			/** output mode */
			int		mode = {0};
		};

		/** log handle type */
		typedef uint32_t (*log_handle_t)(int logfd, LogLevel level, LogParam* param, const char* string);

		/**
		 * default logging index
		 **/
		enum LogIndex {
			c_log_index_invalid = -1,
			c_log_index_start	= ::common::LOG_START,
			c_log_index_max		= 10,
		};

		/** max single log length */
		static const int c_log_max_size  = c_length_64K;
		/** logging file size */
		static const int c_logging_size  = c_length_64M * 2;
		/** logging file count */
		static const int c_logging_count = 5;
		/** default log path */
	    static const char* c_log_path;
	    /** default log name */
	    static const char* c_log_name;

		/**
		 * log limit
		 **/
		struct LogLimit {
			int64_t size  = {c_logging_size};
			int64_t count = {c_logging_count};
		};

		/** logger instance */
		static Logging s_instance[c_log_index_max];

	public:
		/**
		 * set current log level
		 */
		static void	set_level(LogLevel level, int index = c_log_index_start) {
			s_instance[index].m_level = level;
		}

		/**
		 * set current log name
		 */
		static void	set_name(const std::string& name = "", const std::string& path = "", int index = c_log_index_start);

		/**
		 * set current log size
		 * */
		static void set_mode(int dest = 0, int mode = 0, const char* prefix = NULL, int index = c_log_index_start);

		/**
		 * set current log size
		 * */
		static void set_limit(int64_t size = 0, int count = 0, int index = c_log_index_start);

	public:
		/**
		 * set current log name
		 */
		void		log_name(const std::string& _name, bool suffix = true);

		/**
		 * get current log name
		 **/
		const std::string& log_name() { return m_name; }

		/**
		 * get current log handler
		 * */
		log_handle_t log_handler() { return m_handle; }

		/**
		 * set current log handle
		 **/
		void		log_handle(log_handle_t _handle) { m_handle = _handle; }

		/**
		 * set current log level
		 */
		void		log_level(LogLevel _level) { m_level = _level; }

		/**
		 * get current log level
		 **/
		common::LogLevel log_level() { return m_level; }

		/**
		 * log format helper
		 **/
		void		log_format(LogLevel level, const char* filename, int line, const char *fmt, ...);

		/**
		 * format logger
		 **/
		void		format(LogLevel level, const char* filename, int line, const char* string) {
		    m_param.filename = filename;
		    m_param.line = line;

			return format(level, string);
		}

		/**
		 * format logger
		 **/
		void		format(LogLevel level, const char* string, bool lock = true);

	protected:

	    struct LogVec;

		/**
		 * get logger path
		 **/
		std::string get_path(int num = 0);

		/**
		 * initialize logger
		 **/
		void		initialize();

		/**
		 * get initialize state
		 **/
		bool		initialized() { return m_initial; }

		/**
		 * roll logger
		 **/
		void		roll();

		/**
		 * close logger
		 **/
		void		close();

	protected:
		/** initialize state */
		bool		m_initial = {false};
		/** logger m_index */
		int			m_index   = {c_log_index_invalid};
		/** open file handle */
		int			m_logfd	  = {c_invalid_handle};
		/** log file m_length */
		int64_t		m_length  = {0};
		/** log handle */
		log_handle_t m_handle = {NULL};
		/** log path */
		std::string	m_path 	= {c_log_path};
		/** log name */
		std::string	m_name	= {c_log_name};
		/** log level */
		LogLevel 	m_level = {LOG_LEVEL_DEBUG};
		/** log limit */
		LogLimit	m_limit;
		/** log bitmap */
		LogParam 	m_param;
		/** log file vector */
		LogVec*		m_logvec = {NULL};
	};
}
using ::common::Logging;

/** set log level helper */
#define set_log_level(level, index) 	\
	Logging::set_level(::common::LOG_LEVEL_##level, index)

/** c style log mode */
#define __VAR_LOG(index, level, fmt, arg...)				\
	do {													\
		Logging& logger = common::Logging::s_instance[index];	\
		if (::common::LOG_LEVEL_##level >= logger.log_level()) {\
			logger.log_format(::common::LOG_LEVEL_##level,		\
					__FILE__, __LINE__, fmt, ##arg);		\
		}													\
	} while (0)

/** cpp style log mode */
#define __BASE_LOG(index, level, display) 					\
	do {													\
		Logging& logger = Logging::s_instance[index];\
		if (::common::LOG_LEVEL_##level >= logger.log_level()) {\
			std::stringstream __stream;						\
			__stream << display;							\
			logger.format(::common::LOG_LEVEL_##level,		\
				__FILE__, __LINE__, __stream.str().c_str());\
		}													\
	} while (0)

/** helper for log m_index start */
#define VAR_LOG(level, fmt, arg...)		__VAR_LOG(LOG_START, fmt, ##arg)
#define BASE_LOG(level, display)		__BASE_LOG(LOG_START, level, display)

/** helper for log level */
#define VAR_CODE(index, fmt, arg...)	__VAR_LOG(index, CODE,  fmt, ##arg)
#define VAR_TRACE(index, fmt, arg...)	__VAR_LOG(index, TRACE, fmt, ##arg)
#define VAR_DEBUG(index, fmt, arg...)	__VAR_LOG(index, DEBUG, fmt, ##arg)
#define VAR_INFO(index, fmt, arg...)	__VAR_LOG(index, INFO,  fmt, ##arg)
#define VAR_WARN(index, fmt, arg...)	__VAR_LOG(index, WARN,  fmt, ##arg)
#define VAR_ERROR(index, fmt, arg...)	__VAR_LOG(index, ERROR, fmt, ##arg)
#define VAR_FATAL(index, fmt, arg...)	__VAR_LOG(index, FATAL, fmt, ##arg)

#define BASE_CODE(index, info)		__BASE_LOG(index, CODE, info)
#define BASE_TRACE(index, info)		__BASE_LOG(index, TRACE, info)
#define BASE_DEBUG(index, info)		__BASE_LOG(index, DEBUG, info)
#define BASE_INFO(index, info)		__BASE_LOG(index, INFO,  info)
#define BASE_WARN(index, info)		__BASE_LOG(index, WARN,  info)
#define BASE_ERROR(index, info)		__BASE_LOG(index, ERROR, info)
#define BASE_FATAL(index, info) 	__BASE_LOG(index, FATAL, info)

	/** current log index */
	#define LOG_INDEX ::common::LOG_START

	#define var_code(fmt, arg...) 	VAR_CODE(LOG_INDEX, fmt, ##arg)
	#define var_trace(fmt, arg...) 	VAR_TRACE(LOG_INDEX, fmt, ##arg)
	#define var_debug(fmt, arg...)	VAR_DEBUG(LOG_INDEX, fmt, ##arg)
	#define var_info(fmt, arg...)	VAR_INFO(LOG_INDEX,  fmt, ##arg)
	#define var_warn(fmt, arg...)	VAR_WARN(LOG_INDEX,  fmt, ##arg)
	#define var_error(fmt, arg...)	VAR_ERROR(LOG_INDEX, fmt, ##arg)
	#define var_fatal(fmt, arg...) 	VAR_FATAL(LOG_INDEX, fmt, ##arg)

	#define log_code(info) 			BASE_CODE(LOG_INDEX, info)
	#define log_trace(info) 		BASE_TRACE(LOG_INDEX, info)
	#define log_debug(info)	 		BASE_DEBUG(LOG_INDEX, info)
	#define log_info(info)	 		BASE_INFO(LOG_INDEX,  info)
	#define log_warn(info)	 		BASE_WARN(LOG_INDEX,  info)
	#define log_error(info)	 		BASE_ERROR(LOG_INDEX, info)
	#define log_fatal(info) 		BASE_FATAL(LOG_INDEX, info)
