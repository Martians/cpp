
#pragma once

#include <cstddef>
#include <cassert>

namespace common {

	namespace inner {
		/** inner assert string */
		#define assert_fail(string)	__assert_fail(string, __FILE__, __LINE__, __ASSERT_FUNCTION)

		/**
		 * not implement yet
		 * */
		#define assert_later(value)	assert_fail(#value " not implement yet")

		/** trigger fault format string */
		#define	fault_format(fmt, append, arg...)	do { 			        \
			std::string _export = common::format(fmt "%s", ##arg, append);  \
			assert_fail(_export.c_str()); 							        \
		} while (0)
	}

	/** must be success, or assert */
	#define success(value)	do { if (!(value)) { assert_fail(#value); } } while (0)

	/** must be success, or assert */
	#define execute(ptr, task)	if (ptr) { ptr->task; }

	/**
	 * @brief get array size
	 */
//	#define	array_size(x)	sizeof(x)/sizeof((x)[0])

	/**
	 * @brief give raise to core
	 */
	#define	core_dump		do { int* x = 0; int y = *x; y += 1; } while (0)

	/**
	 * get member offset
	 **/
	#define OFFSET(T, m) 	(int)(((long)&((T*)1)->m) - 1)

	#ifndef container_of
		/**
		 * @brief get struct ptr from member ptr
		 * @ptr		member pointer
		 * @type	struct type name
		 * @member	struct member name
		 */
		#define container_of(ptr, type, member)	\
				(type *)((char *)ptr - OFFSET(type, member))
	#endif

	/**
	 * @brief get high int32_t from int64_t
	 */
	#define high_64(x)		((int32_t)((0xFFFFFFFF00000000LL & (x)) >> 32))

	/**
	 * @brief get low  int32_t from int64_t
	 */
	#define low_64(x)		((int32_t)(0x00000000FFFFFFFFLL & (x)))

	/**
	 * @brief get high int32_t from int64_t
	 */
	#define int64(a, b)		(((int64_t)(a) << 32) | (b))

	/**
	 * @brief get int64_t whose high int32_t seted, orign not changed
	 */
	#define set_high(x,a) 	(((x) & 0x00000000FFFFFFFFLL) | ((int64_t)(a) << 32))

	/**
	 * @brief get int64_t whose low  int32_t seted, orign not changed
	 */
	#define set_low(x,a)	(((x) & 0xFFFFFFFF00000000LL) | (int64_t)(a))
}

