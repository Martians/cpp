
#pragma once

#include <string>

#include "Common/Type.hpp"
#include "Common/Define.hpp"
#include "Advance/BaseBuffer.hpp"

namespace common {

	struct BufferTrigger {
		int 	type = {0};
	};
	const BufferTrigger PackEnd = {};

	/** 
	 * @brief input buffer tag
	 * @note trigger buffer event
	 */
	inline void operator << (BaseBuffer& buffer, const BufferTrigger& trigger) {
		buffer.trigger(trigger.type);
	}

	/** 
	 * @brief input data
	 * @note trigger buffer event
	 */
	struct BufferWrap {
	public:
		explicit BufferWrap(void* data, length_t size, bool set = false)
			: _data(data), _size(size), _set(set) {}
	public:
        /** data start */
		void*	_data;	
		/** data len */
		length_t _size;
		/** set len or not */
		bool	_set;
	};

	/** define pack wrap */
	typedef struct BufferWrap PackWrap;

	/** 
	 * @brief packet data wrap
	 */
	inline BaseBuffer& operator << (BaseBuffer& packet, const PackWrap& wrap) {
		if (wrap._set) {
			packet.write_int32(wrap._size);
		}
		length_t len = packet.write(wrap._data, wrap._size);
		success(len == wrap._size);
		return packet;
	}

	/** 
	 * @brief output bool with swap
	 */
	inline BaseBuffer& operator >> (BaseBuffer& buffer, bool& type) {
		type = buffer.read_bool();
		return buffer;							
	}

	/** 
	 * @brief input bool with swap
	 */
	inline BaseBuffer& operator << (BaseBuffer& buffer, bool type) {
		buffer.write_bool(type);
		return buffer;							
	}

	/** 
	 * @brief input std::string
	 */
	inline BaseBuffer& operator << (BaseBuffer& buffer, const std::string& str) {
		buffer.write_string(&str);
		return buffer;										
	}

	/** 
	 * @brief output std::string
	 */
	inline BaseBuffer& operator >> (BaseBuffer& buffer, std::string& s) {
		buffer.read_string(&s);				
		return buffer;									
	}

	/** 
	 * read string
	 **/
	length_t 	read_string(BaseBuffer& buffer, std::string& str) {
		return buffer.read_string(&str);
	}

	/**
	 * read string
	 **/
	length_t    write_string(BaseBuffer& buffer, std::string& str) {
		return buffer.write_string(&str);
	}

	/**
	 * @brief input const char
	 */
	inline BaseBuffer& operator << (BaseBuffer& buffer, const char* str) {
		length_t len = (length_t)strlen(str);
		buffer.write_int32(len);			
		buffer.write(str, len);
		return buffer;									
	}

	/** 
	 * @brief output const char
	 * @note buffer should big enough
	 */
	inline BaseBuffer& operator >> (BaseBuffer& buffer, char* str) {
		length_t slen = buffer.read_int32();		
		length_t len = buffer.read(str, slen);
		success(len == slen);			
		str[len] = '\0';
		return buffer;		
	}

	/** 
	 * @brief common input
	 */
	#define DEFINE_BUFFER_IN(BUFFER, TYPE) inline BUFFER& operator << (BUFFER& buffer, TYPE type) {\
		length_t ret = buffer.write(&type, sizeof(type));\
		success(ret == sizeof(type));			\
		return buffer;							\
	}

	/** 
	 * @brief common output
	 */
	#define DEFINE_BUFFER_OUT(BUFFER, TYPE) inline BUFFER& operator >> (BUFFER& buffer, TYPE& type) {\
		length_t ret = buffer.read(&type, sizeof(type));\
		success(ret == sizeof(type));			\
		return buffer;							\
	}

	#define DEFINE_BUFFER_IO(BUFFER, TYPE)		\
		DEFINE_BUFFER_IN(BUFFER, TYPE)			\
		DEFINE_BUFFER_OUT(BUFFER, TYPE)

	/** 
	 * @brief input with swap
	 */
	#define DEFINE_BUFFER_IN_SWAP(BUFFER, TYPE) inline BUFFER& operator << (BUFFER& buffer, TYPE type) {\
        buffer.swap_byte(&type, sizeof(type));	\
		length_t ret = buffer.write(&type, sizeof(type));\
		success(ret == sizeof(type));			\
		return buffer;							\
	}

	/** 
	 * @brief output with swap
	 */
	#define DEFINE_BUFFER_OUT_SWAP(BUFFER, TYPE) inline BUFFER& operator >> (BUFFER& buffer, TYPE& type) {\
		length_t ret = buffer.read(&type, sizeof(type));\
		buffer.swap_byte(&type, sizeof(type));	\
		success(ret == sizeof(type));			\
		return buffer;							\
	}

	#define DEFINE_BUFFER_IO_SWAP(BUFFER, TYPE) \
		DEFINE_BUFFER_IN_SWAP(BUFFER, TYPE)     \
		DEFINE_BUFFER_OUT_SWAP(BUFFER, TYPE)

    /** 
	 * @brief input buffer
	 */
	#define DEFINE_BUFFER_IN_BUFFER(BUFFER) inline BUFFER& operator << (BUFFER& buffer, BUFFER& v) {\
		buffer.copy(&v);						\
		return buffer;							\
	}

   /** 
	 * @brief output buffer
	 */
	#define DEFINE_BUFFER_OUT_BUFFER(BUFFER) inline BUFFER& operator >> (BUFFER& buffer, BUFFER& v) {\
		length_t len = v.copy(&buffer);			\
		buffer.remove(len);						\
		return buffer;							\
	}

	#define DEFINE_BUFFER_IO_BUFFER(BUFFER)		\
		DEFINE_BUFFER_IN_BUFFER(BUFFER)			\
		DEFINE_BUFFER_OUT_BUFFER(BUFFER)

	/** 
	 * @brief io func sets
	 */
	#define	DEFINE_BUFFER_IO_SETS(BUFFER)		\
		DEFINE_BUFFER_IO(BUFFER, byte_t)		\
		DEFINE_BUFFER_IO_SWAP(BUFFER, int16_t)	\
		DEFINE_BUFFER_IO_SWAP(BUFFER, uint16_t)	\
		DEFINE_BUFFER_IO_SWAP(BUFFER, uint32_t)	\
		DEFINE_BUFFER_IO_SWAP(BUFFER, length_t)	\
		DEFINE_BUFFER_IO_SWAP(BUFFER, int64_t)	\
		DEFINE_BUFFER_IO_SWAP(BUFFER, uint64_t)	\
	  /*DEFINE_BUFFER_IO_SWAP(BUFFER, bool)*/	\
		DEFINE_BUFFER_IO_BUFFER(BUFFER)
		
	DEFINE_BUFFER_IO_SETS(BaseBuffer);	
}
