
#pragma once

#include "Advance/DynBuffer.hpp"

namespace common {

/**
 * @brief data chunk used for dyn buffer
 * @note not check opt length in chunk
 */
struct DynBuffer::Chunk
{
public:
	Chunk(byte_t* _data = NULL) : data(_data) {}

	Chunk(const Chunk& v) {
		operator = (v);
	}

	const Chunk& operator = (const Chunk& v) {
		assign(const_cast<Chunk*>(&v));
		return *this;
	}

public:
	/**
	 * @brief format new chunk by data
	 * @param data to format chunk
	 */
	static Chunk* _new(void* data);

	/**
	 * @brief clear data
	 */
	void		clear() { beg = end = 0; }

	/**
	 * get total length
	 **/
	length_t	total() const;

	/**
	 * @brief the length data holded by chunk
	 */
	length_t	length() { return end - beg; }

	/**
	 * @brief check if chunk is full
	 */
	bool		full() { return end >= total(); }

	/**
	 * @brief check if chunk is empty
	 */
	bool		empty() { return beg >= end; }

	/**
	 * @brief remain len for new data
	 */
	length_t	remain() { return total() - end; }

	/**
	 * @brief buffer position for read
	 */
	byte_t*		rpos()  { return data + beg; }

	/**
	 * @brief buffer position for write
	 */
	byte_t*		wpos() { return data + end; }

	/**
	 * @brief inc data len
	 * @note not check length
	 */
	void		inc(length_t len) { end += len; }

	/**
	 * @brief dec data len
	 * @note not check length
	 */
	void		dec(length_t len) { beg += len; }

	/**
	 * @brief retrieve data from end
	 * @note not check length
	 */
	length_t	retrieve(length_t len) { end -= len; return len; }

	/**
	 * @brief operator []
	 * @return -1 if exceed data length
	 */
	char 		operator[] (length_t pos) const { return pos >= total() ? -1 : *(data + pos); }

	/**
	 * @brief assign data froom another chunk
	 * @param chunk the src chunk
	 * @param len assign len, default is the src chunk data len
	 */
	void		assign(Chunk* chunk, length_t len = BaseBuffer::c_invalid_length);

	/**
	 * @brief adjust data to chunk head
	 */
	void		adjust();

public:
	/** data start off */
	length_t	beg = {0};
	/** data end off */
	length_t	end = {0};
	/** next chunk pointer */
	Chunk*		next = {NULL};
	/** data start */
	byte_t*		data = {NULL};
};

}
