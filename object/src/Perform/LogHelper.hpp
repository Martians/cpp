
/** should not use this */
//#pragma once

/**
 * if you only use log_code in hpp,
 * 		only need #include "Common/LogHelper.hpp"
 */

/** define this at the beginning of hpp, when you want use log_code in hpp file */
#ifdef LOG_CODE_HPP
	#undef 	debug
	#undef 	trace

	/** define this at the ending of hpp, when you already use log_code in beginning */
	#ifndef LOG_CODE_HPP_END

		/** keep origin define */
		#if LOG_CODE
		#	define	ORIGIN_LOG 1
		#else
		#	define	ORIGIN_LOG 0
		#endif
		#undef  LOG_CODE
		#define	LOG_CODE	LOG_CODE_HPP

		/** define this, if you want use another log in hpp, can only used once at same time */
		#ifdef CHANGE_INDEX
			#include "Common/Logger.hpp"
			/* disable current redirect */
			const static int s_log_index = LOG_INDEX;
			#undef 	LOG_INDEX
			#define LOG_INDEX CHANGE_INDEX
		#endif

	#else
		/** recover origin define */
		#undef	LOG_CODE
		#if ORIGIN_LOG
		#	define LOG_CODE 1
		#else
		#	define LOG_CODE 0
		#endif
		#undef 	ORIGIN_LOG

		#undef	LOG_CODE_HPP
		#undef	LOG_CODE_HPP_END

		#ifdef CHANGE_INDEX
			/* recover current redirect */
			#undef 	LOG_INDEX
			#define LOG_INDEX s_log_index
			#undef 	CHANGE_INDEX
		#endif
	#endif
#endif

#include "Common/LogHelper.hpp"


