

#pragma once

#include <string>

#include "Common/Const.hpp"

namespace common {

	#if USING_UUID
	/**
	 * get uuid string
	 **/
	void		get_uuid(byte_t* data);

	/**
	 * get uuid string
	 **/
	std::string	get_uuid();
	#endif

}
