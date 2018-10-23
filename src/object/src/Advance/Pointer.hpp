
#pragma once

#include <cstring>

#include "Common/Type.hpp"
#include "Common/Const.hpp"
#include "Advance/Util.hpp"

namespace common {

	/**
	 * write data and increase pointer
	 **/
	template<typename Type>
	Type* 	write_data(Type*& data, const void* temp, uint32_t len) {
		memcpy(data, temp, len);
	    data = (byte_t*)data + len;
	    return data;
	}

	/**
	 * get aligned address
	 **/
	template<class Type, class Tick>
	Type	address_align(const Type& data, Tick align) {
		return (Type)up_align((int64_t)data, (int64_t)align);
	}

	/**
	 * align type with page_size
	 **/
	template<class Type>
	Type 	page_align(Type data) {
		return address_align(data, c_page_size);
	}

	/**
	 * malloc aligned address
	 *
	 * @param data new data start
	 * @param algin aligned length
	 * @param len total data length
	 **/
	template<class Type, class Tick>
	Type* 	malloc_align(Type*& data, uint32_t len, Tick align) {
		data = (Type*)(new byte_t[len + align]);
		return address_align(data, align);
	}
	
    /**
	 * get aligned address on stack
	 **/
	#define	stack_align(name, len, align) 				\
			byte_t temp[len + align] = {};				\
			byte_t* name = address_align((byte_t*)temp, align)

}

