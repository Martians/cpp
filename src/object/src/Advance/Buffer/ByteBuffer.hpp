
#pragma once

#include "Advance/BaseBuffer.hpp"

namespace common
{
	/** 
	 * @brief simple buffer, can only used for a time,
	 * if use in cycle, should clear first
	 */
	class ByteBuffer : public BaseBuffer
	{
	public:
		/**
		 * @param len buffer length
		 * @note alloc new data in heep
		 */
		ByteBuffer(length_t size = 0) { malloc(size); }

		/**
		 * @param data buffer start
		 * @param len buffer length
		 * @param release buffer or not
		 */
		ByteBuffer(void* data, length_t len, bool owner = true) {
			attach(data, len, owner);
		}

		virtual ~ByteBuffer() { clear(true); }

	public:
		/** 
		 * @brief write given data to buffer
		 * @param data source data address
		 * @param len write len
		 * @return actually writted data len
		 */
		virtual length_t write(const void* data, length_t len);

		/**
		 * @brief handle data
		 * @param handle function
		 * @param ptr handle param
		 * @param len handle data length
		 * @param off handle data offset
		 **/
		virtual length_t dispatch(data_handle_t handle, void* ptr,
					length_t len = BaseBuffer::c_invalid_length, length_t off = 0) const;

		/** 
		 * @brief remove data from buffer
		 * @param len remove len
		 * @note free space not in used
		 */
		virtual length_t remove(length_t len) {
			len = BaseBuffer::remove(len);
			m_start	+= len; 
			return len;
		}

		/** 
		 * @brief buffer remain len
		 */
		//virtual length_t remain() { return m_total - m_start - m_length; }

	public:
		/**
		 * @brief set data buffer
		 * @param data data start
		 * @param len  data len
		 * @param release data when deconstruct
		 */
		void		attach(void* data, length_t len, bool owner = true);

		/**
		 * @brief reuse buffer for read
		 */
		void		reset(length_t len = 0) {
			m_length = len;
			m_start	= 0;
		}

		/**
		 * @brief clear buffer state
		 */
		void		clear(bool cycle = false);

		/** 
		 * @brief write pos
		 */
		byte_t*		wpos() { return m_data + m_start + m_length; }

		/** 
		 * @brief read pos
		 */
		byte_t*		rpos(length_t pos = 0) const { return m_data + m_start + pos; }

		/** 
		 * @brief alloc new buffer space
		 */
		void		malloc(length_t len);

	public:
		/** buffer data start */
		byte_t*		m_data	= {NULL};
		/** data start off */
		length_t	m_start = {0};
	};
}


