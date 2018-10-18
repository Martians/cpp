
#include <cstring>
#include <algorithm>

#include "Common/Define.hpp"
#include "Advance/Util.hpp"
#include "Advance/Pointer.hpp"
#include "Advance/Buffer/ByteBuffer.hpp"

namespace common {

void
ByteBuffer::clear(bool cycle)
{
	m_length = 0;
	m_start = 0;

	if (cycle) {
		if (reset_bit(BT_owner)) {
			reset_array(m_data);
		} else {
			m_data = NULL;
		}
	}
}

void
ByteBuffer::malloc(length_t len)
{
	if (len > 0) {
		len = up_round(len, ByteBuffer::c_align_boundray);
		byte_t* data = new byte_t[len];
		attach(data, len);
	}
}

void	
ByteBuffer::attach(void* data, length_t len, bool owner)
{
	clear(true);

	set_bit(BT_owner, owner);
	m_data	= (byte_t*)data;
	m_total = len;
}

length_t
ByteBuffer::write(const void* data, length_t len)
{
	success(len <= remain());
	memcpy(wpos(), data, len);
	inc(len);
	return len;
}

int
ByteBuffer::dispatch(data_handle_t handle, void* ptr,
	length_t len, length_t off) const
{
	len = std::min(length() - off, len);
	length_t ret = handle(ptr, rpos() + off, len);

	if (ret < 0) {
		return ret;
	} else {
		return len;
	}
}

}

