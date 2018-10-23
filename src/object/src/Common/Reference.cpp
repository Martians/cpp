

#if USING_UUID
#	include <uuid/uuid.h>
#endif

#include "Common/Const.hpp"
#include "Common/Reference.hpp"

namespace common {
#if USING_UUID
	std::string
	get_uuid()
	{
		uuid_t uuid;

		byte_t data[64];
		uuid_generate(uuid);
		uuid_unparse(uuid, data);
		return data;
	}

	void
	get_uuid(byte_t* data)
	{
		uuid_t uuid;
		uuid_generate(uuid);
		uuid_unparse(uuid, data);
}
#endif
}
