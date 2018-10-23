
#pragma once

#include "Common/Type.hpp"
#include "Common/Const.hpp"
#include "CodeHelper/Validate.hpp"
#include "CodeHelper/Bitset.hpp"
#include "Advance/ByteOrder.hpp"

namespace common {

	/** 
	 * @brief base BaseBuffer, define base operator 
	 * @param we not throw exception when read or write beyond len limit
	 */
	class BaseBuffer
	{
	public:
		BaseBuffer() {}
		virtual ~BaseBuffer() {}	

		/** buffer tag */
		enum BufferTag {
			BT_null    = 0,
			/** convert byte order or not */
			BT_convert,
			/** release buffer or not */
			BT_owner,
			/** buffer alloced by alloter */
			BT_alloter,
		};

		/**
		 * data process handle dispatch to buffer
		 *
		 * @param ptr for handle param
		 * @param data write buffer start
		 * @param len write buffer length
		 **/
		typedef length_t (*data_handle_t)(void* ptr, const byte_t* data, length_t len);

	public:
		/** 
		 * @brief write given data to buffer
		 * @param data source data address
		 * @param len write len
		 * @return actually writted data len
		 */
		virtual length_t write(const void* data, length_t len) = 0;

		/**
		 * @brief handle data
		 * @param handle function
		 * @param ptr handle param
		 * @param len handle data length
		 * @param off handle data offset
		 **/
		virtual length_t dispatch(data_handle_t handle, void* ptr,
					length_t len = BaseBuffer::c_invalid_length, length_t off = 0) const = 0;

		/** 
		 * @brief remove data from buffer
		 * @param len remove len
		 * @return actually remove length
		 * @note mostly, free space will not be used again
		 */
		virtual length_t remove(length_t len = BaseBuffer::c_invalid_length) {
			if (len > m_length) {
				len = m_length;
			}
			dec(len);
			return len;
		}

		/** 
		 * @brief occur tag and do sth
		 */
		virtual void trigger(int type) {}

	public:
		/**
		 * @brief peek buffer data to given address
		 * @param data the dest address
		 * @param len peek len
		 * @param off peek off from current buffer data start
		 * @return actually readed data len
		 * @note data remain in buffer, none state has changed
		 */
		length_t 	peek(void* data, length_t len, length_t off = 0) const;

		/**
		 * @brief peek local data and append to given buffer
		 * @param buffer the dst buffer
		 * @param len append len
		 * @param check byte order or note
		 * @note when two buffer content in same packet, must check byte order
		 * @note len must smaller than local buffer data length and dst buffer
		 * remain length
		 * @note just append, will not change local buffer length
		 */
		length_t 	peek(BaseBuffer* buffer, length_t len =
						BaseBuffer::c_invalid_length, bool check = true, length_t off = 0) const;
		/**
		 * @brief find std::string in buffer
		 * @param data src address
		 * @param len match len if data BaseBuffer
		 * @param find start from current buffer data start
		 * @return the start position that match data + len
		 */
		length_t	find(const void* data, length_t len, length_t off = 0) const;

		/**
		 * @brief append given buffer data 
		 * @param buffer the src buffer
		 * @param len copy len
		 * @param check byte order or note
		 * @note when two buffer content in same packet, must check byte order
		 * @note just append, will not change given buffer length
		 */
		length_t	copy(BaseBuffer* buffer, length_t len = BaseBuffer::c_invalid_length,
						bool check = true) { return buffer->peek(this, len, check); }
		/** 
		 * @brief read data to dst BaseBuffer
		 * @param data dst BaseBuffer start
		 * @param len try to read len
		 * @return actually readed data len
		 * @note not support read from any off
		 */
		length_t	read(void* data, length_t len) {
			length_t ret = peek(data, len);
			remove(ret);
			return ret;
		}

		/** 
		 * @brief read a 8 bit byte
		 * @param clear or keep data
		 * @return value
		 */
		byte_t		read_byte(bool clear = true) {
			byte_t value = 0;
			peek(&value, sizeof(value));
			if (clear) {
				remove(sizeof(value));
			}
			return value;
		}
	
		/** 
		 * @brief read 16 bit short int
		 * @param clear or keep data
		 * @return value
		 * @note convert byte order if needed
		 */
		int16_t		read_int16(bool clear = true) {
			int16_t value = 0;
			peek(&value, sizeof(value));
			if (clear) {
				remove(sizeof(value));
			}
			return cvt_byte() ? swap_16(value) : value;
		}

		/** 
		 * @brief read 32 bit int
		 * @param clear or keep data
		 * @return value
		 * @note convert byte order if needed
		 */
		length_t	read_int32(bool clear = true) {
			length_t value = 0;
			peek(&value, sizeof(value));
			if (clear) {
				remove(sizeof(value));
			}
			return cvt_byte() ? swap_32(value) : value;
		}

		/** 
		 * @brief read 64 bit int
		 * @param clear or keep data
		 * @return value
		 * @note convert byte order if needed
		 */
		int64_t		read_int64(bool clear = true) {
			int64_t value = 0;
			peek(&value, sizeof(value));
			if (clear) {
				remove(sizeof(value));
			}
			return cvt_byte() ? swap_64(value) : value;
		}

		/** 
		 * @brief read bool as 32 bit int
		 * @return actually writted std::string len
		 * @note convert byte order if needed
		 */
		bool		read_bool() { return read_byte() == 1; }

		/** 
		 * @brief write 8 bit byte
		 * @return actually writted len
		 */
		length_t	write_byte(byte_t value) {
			return write(&value, sizeof(value));
		}

		/** 
		 * @brief write 16 bit short int
		 * @return actually writted len
		 * @note convert byte order if needed
		 */
		length_t	write_int16(int16_t value) {
			value = cvt_byte() ? swap_16(value) : value;
			return write(&value, sizeof(value));
		}

		/** 
		 * @brief write 32 bit int
		 * @return writted count
		 * @note convert byte order if needed
		 */
		length_t	write_int32(length_t value) {
			value = cvt_byte() ? swap_32(value) : value;
			return write(&value, sizeof(value));
		}

		/** 
		 * @brief write 64 bit int
		 * @return actually writted std::string len
		 * @note convert byte order if needed
		 */
		length_t	write_int64(int64_t value) {
			value = cvt_byte() ? swap_64(value) : value;
			return write(&value, sizeof(value));
		}

		/** 
		 * @brief write bool as 32 bit int
		 * @return actually writted std::string len
		 * @note convert byte order if needed
		 */
		length_t	write_bool(bool value) { return write_byte(value ? 1 : 0); }

		/** 
		 * @brief read std::string from dyn BaseBuffer
		 * @param s the dst std::string
		 * @param peek not remove
		 * @return actually readed std::string len
		 * @note peek opt is incomplete
		 */
		length_t	read_string(void* str);

		/** 
		 * @brief write std::string to dyn BaseBuffer
		 * @param s src std::string
		 * @return total writted data len
		 */
		length_t	write_string(void const* str);

	public:
		/**
		 * @brief current data length
		 */
		length_t	length() const { return m_length; }

		/**
		 * @brief set data length
		 * @note should be careful when load this, we should write data firset
		 *then set data length
		 */
		void		length(length_t len) { m_length = len; }

		/**
		 * @brief inc data length
		 */
		void		inc(length_t len) { m_length += len; }

		/**
		 * @brief dec data length
		 */
		void		dec(length_t len) { m_length -= len; }

	public:
		/**
		 * @brief buffer total len
		 */
		length_t	total() { return m_total; }

		/**
		 * @brief get buffer remain len to hold data
		 */
		length_t 	remain() {
			if (m_total == BaseBuffer::c_invalid_length) {
				return m_total;

			} else {
				return m_total - m_length;
			}
		}

		/**
		 * @brief retrieve last space in buffer, from end to start
		 * @param len retrieve len
		 * @return actually remove length
		 * @note free space not in used
		 */
		length_t	retrive(length_t len) {
			if (len > m_length) {
				len = m_length;
			}
			dec(len);
			return len;
		}

	public:
		/**
		 * @brief current use nbo or not
		 */
		bool		nbo() { return c_local_bod ? !cvt_byte() : cvt_byte(); }

		/**
		 * @brief set current use nbo or not
		 */
		void		nbo(bool set) { cvt_byte(set ? !c_local_bod : c_local_bod); }

		/** 
		 * @brief convert endian or not
		 */
		bool		cvt_byte() const { return get_bit(BT_convert); }

		/** 
		 * @brief set convert endian 
		 */
		void		cvt_byte(bool v) { set_bit(BT_convert, v); }

		/**
		 * convert byte
		 **/
		void		swap_byte(void* type, int size) {
			if (cvt_byte()) {
				common::swap_byte(type, size);
			}
		}

        /** invalid len */
		static const length_t c_invalid_length = INT32_MAX;
		/** boundary limit */

		static const length_t c_align_boundray = 256;

	protected:

		VALIDATE_DEFINE;

		BITSET_DEFINE;

	protected:
		/** current data length */
		length_t	m_length = {0};
		/** buffer total length */
		length_t	m_total  = {c_invalid_length};
	};
}
