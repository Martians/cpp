
#pragma once

#include "Advance/Buffer/ByteBuffer.hpp"

namespace common
{
	class CycleBuffer : public ByteBuffer
	{
	public:
		CycleBuffer();
		virtual ~CycleBuffer();

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
			len = ByteBuffer::remove(len);
			if (m_start >= m_total) {
				m_start -= m_total;
			}
			return len;
		}

		/** 
		 * @brief clear buffer state
		 * @note move data to head
		 */
		void		clear() {
			m_length = 0;
			m_end = 0;
		}

		/** 
		 * @brief set new buffer len
		 */
		bool		set_len(length_t len);

	protected:
		length_t	m_end = {0};
	};

}

