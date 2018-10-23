
/**
 * if you will use log_code and log_debug log_info...,
 * 		need #define LOG_CODE 0/1, then #include "Common/LogHelper.hpp"
 **/

#include "Common/Logger.hpp"

#undef 	debug
#undef 	trace

#if LOG_CODE
    #define	debug(x)	log_code(x)
    #define	trace(x)	log_code(x)
#else
	#define	debug(x)
	#define	trace(x)
#endif

