
#pragma once

namespace common {

	/** local indian, nbo or not nbo (lte) */
	extern const int c_local_bod;

	/**
	 * @brief swap 16-bit short int
	 */
	#define swap_16(x)		((((x) & 0xFF00) >> 8) + (((x) & 0x00FF) << 8))

	/**
	 * @brief swap 32-bit int
	 */
	#define swap_32(x)		((((x) & 0xFF000000) >> 24) + (((x) & 0x00FF0000) >> 8) + (((x) & 0x0000FF00) << 8) + (((x) & 0x000000FF) << 24))

	/**
	 * @brief swap 64-bit int
	 */
	#define	swap_64(x)		((swap_32((x) & 0xFFFFFFFF) << 32) | swap_32((x) >> 32))

	/**
	 * @brief swap byte int
	 **/
	void 	swap_byte(void* data, int len);

	/**
	 * @brief swap int type
	 **/
	template<class Type>
	Type 	swap_int(Type type) {
		swap_byte(&type, sizeof(Type));
		return type;
	}

	/**
	 * @brief convert int16 byte order between host and net
	 */
	#define cvt_16(x)		(c_local_bod ? x : swap_16(x))

	/**
	 * @brief convert int32 byte order between host and net
	 */
	#define cvt_32(x)		(c_local_bod ? x : swap_32(x))

	/**
	 * @brief convert int64 byte order between host and net
	 */
	#define cvt_64(x)		(c_local_bod ? x : swap_64(x))

	/**
	 * @brief convert int for 16\32\64 bit
	 */
	#define cvt_int(x)		(c_local_bod ? x : swap_int(x))

	/**
	 * @brief used for local buffer writer, change order or not according to local endian
	 * */
	#define	cvt(nbo)		((nbo) != c_local_bod)
}
