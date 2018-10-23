
#include <cstring>
#include <algorithm>

#include "Common/Define.hpp"
#include "Advance/Buffer/CycleBuffer.hpp"

namespace common {

length_t
CycleBuffer::dispatch(data_handle_t handle, void* ptr,
	length_t len, length_t off) const
{
	if (off >= length()) {
		return 0;
	}
	len = std::min(length() - off, len);

	length_t remain = m_total - m_start;
	/** data before tail match what we need */
	if (off + len <= remain) {
		handle(ptr, rpos(off), len);

	/** we need more data */
	} else if (off < remain) {
		remain -= off;
		handle(ptr, rpos(off), remain);
		handle(ptr, m_data, len - remain);

	/* need data exceed tail, back to head */
	} else {
		handle(ptr, m_data + (off - remain), len);
	}
	return len;
}

length_t
CycleBuffer::write(const void* _data, length_t len)
{
	success(m_length + len <= total());
	length_t remain = m_total - m_end;

    const byte_t* data = (const byte_t*)_data;
	/** data before tail match what we need, maybe end > start or end < start,
		but write end will never exceed remain space */
	if (len <= remain) {
		memcpy(wpos(), data, len);
		m_end += len;

	/** we need write wrap */
	} else {
		memcpy(wpos(), data, remain);
		memcpy(m_data, data + remain, len - remain);
		m_end = len - remain;
	}

	inc(len);
	return len;
}

}
