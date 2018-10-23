
#pragma once

#include "Common/Type.hpp"

namespace common {
	struct Data
	{
	public:
		Data() {}

		Data(const Data& v) {
			operator = (v);
		}

		const Data& operator = (const Data& v) {
			len = v.len;
			ptr = v.ptr;
			return *this;
		}
	public:
		int32_t	len = {0};
		byte_t*	ptr = {NULL};
	};
}
