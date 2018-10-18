
#pragma once

#include <stdint.h>

namespace common {
	/** used for typedef char array */
	typedef char 	byte_t;
	/** unsigned char define */
	typedef unsigned char u_byte_t;
}

//using UINT64_MAX in stdint.h
/** used for printf int64 */
typedef long long long_int;
/** buffer length define */
typedef int32_t length_t;
/** identifier define */
typedef int64_t typeid_t;

#if COMMON_SPACE
	using common::byte_t;
	using common::u_byte_t;
#endif
