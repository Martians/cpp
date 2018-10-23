
#pragma once

#include "Common/LogHelper.hpp"

enum LogType
{
	LT_null = common::LOG_START,
	LT_object,
	LT_reader,
	LT_writer,
};

/** object use sperate logging */
#define OBJECT_LOGGING	1
#if 	OBJECT_LOGGING
	#undef 	LOG_INDEX
	#define LOG_INDEX LT_object
#endif


/** use type log or not */
#define TYPE_LOGGING	0
#if 	TYPE_LOGGING

	#if READER_WORK
		#undef 	LOG_INDEX
		#define LOG_INDEX LT_reader
	#elif WRITER_WORK
		#undef 	LOG_INDEX
		#define LOG_INDEX LT_writer
	#endif
#endif

#define object_trace(info) 		BASE_TRACE(LT_object, info)
#define object_debug(info)	 	BASE_DEBUG(LT_object, info)
#define object_info(info)	 	BASE_INFO(LT_object,  info)
#define object_warn(info)	 	BASE_WARN(LT_object,  info)
#define object_error(info)	 	BASE_ERROR(LT_object, info)
#define object_fatal(info) 		BASE_FATAL(LT_object, info)

#define reader_trace(info) 		BASE_TRACE(LT_reader, info)
#define reader_debug(info)	 	BASE_DEBUG(LT_reader, info)
#define reader_info(info)	 	BASE_INFO(LT_reader,  info)
#define reader_warn(info)	 	BASE_WARN(LT_reader,  info)
#define reader_error(info)	 	BASE_ERROR(LT_reader, info)
#define reader_fatal(info) 		BASE_FATAL(LT_reader, info)

#define writer_trace(info) 		BASE_TRACE(LT_writer, info)
#define writer_debug(info)	 	BASE_DEBUG(LT_writer, info)
#define writer_info(info)	 	BASE_INFO(LT_writer,  info)
#define writer_warn(info)	 	BASE_WARN(LT_writer,  info)
#define writer_error(info)	 	BASE_ERROR(LT_writer, info)
#define writer_fatal(info) 		BASE_FATAL(LT_writer, info)
