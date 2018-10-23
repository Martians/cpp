
#include "Common/Type.hpp"
#include "Advance/ByteOrder.hpp"

namespace common {
	void swap_byte(void* data, int len) {
		switch (len) {
		case 2:
			*(int16_t*)data = swap_16(*(int16_t*)data);
			break;
		case 4:
			*(int32_t*)data = swap_32(*(int32_t*)data);
			break;
		case 8:
			*(int64_t*)data = swap_64(*(int64_t*)data);
			break;
		default:
			break;
		}
	}

}
