
#include <string>

#include "Common/Define.hpp"
#include "Advance/Pointer.hpp"
#include "Advance/BaseBuffer.hpp"

namespace common {

VALIDATE(BaseBuffer) {
	length_t  v = c_invalid_length;
	success(v + 1 < c_invalid_length);
}

#if 0
std::string
BaseBuffer::read_string()
{
	std::string str;
	read_string(str);
	return str;
}
#endif

length_t
BaseBuffer::peek(void* data, length_t len, length_t off) const
{
	struct Wrap {
		Wrap(void* _data) : data(_data) {};
		void* data;
	};
	static auto handle = [](void* ptr, const byte_t* data, length_t len) {
		Wrap* wrap = (Wrap*)ptr;
		write_data(wrap->data, data, len);
		return len;
	};
	Wrap wrap(data);
	length_t ret = dispatch(handle, &wrap, len, off);
	success(len == c_invalid_length || len == ret);
	return ret;
}

length_t
BaseBuffer::peek(BaseBuffer* buffer, length_t len, bool check, length_t off) const
{
	/** check byte order */
	if (check) {
		success(cvt_byte() == buffer->cvt_byte());
		/** dst buffer data remain */
		if (len == c_invalid_length) {
			len = std::min(len, buffer->remain());
		}
	}

	if (off >= length()) {
		return 0;
	}

	static auto handle = [](void* ptr, const byte_t* data, length_t len) {
		BaseBuffer* buffer = (BaseBuffer*)ptr;
		return buffer->write(data, len);
	};
	length_t ret = dispatch(handle, buffer, len, off);
	success(len == c_invalid_length || len == ret);
	return ret;
}

length_t
BaseBuffer::find(const void* data, length_t len, length_t off) const
{
	if (off + len > m_length) {
		return -1;
	}
	struct Wrap {
		Wrap(const void* _data, length_t _len, length_t _off)
			: data((const byte_t*)_data), len(_len), off(_off) {}

		const byte_t* data = {NULL};
		length_t len  = {0};
		length_t off  = {0};
		length_t pos  = {0};
		length_t curr = {0};
	};

	static auto handle = [](void* ptr, const byte_t* data, length_t len) {
		Wrap* wrap = (Wrap*)ptr;

		if (wrap->curr + len > wrap->off) {
			length_t pos = (wrap->curr >= wrap->off) ?
				0 : (wrap->off - wrap->curr);

			while (pos < len) {
				if (*(data + pos) == *(wrap->data + wrap->pos)) {
					if (++wrap->pos == wrap->len) {
						wrap->curr += pos - wrap->pos;
						return -1;
					}
				} else {
					wrap->pos = 0;
				}
				pos++;
			}
		}
		wrap->curr += len;
		return len;
	};

	Wrap wrap(data, len, off);
	dispatch(handle, &wrap, len, off);
	return wrap.pos == wrap.len ? wrap.curr : -1;
}

length_t
BaseBuffer::read_string(void* string)
{
    std::string* str = (std::string*)string;
	length_t length = read_int32();
	success(m_length >= length);
	str->reserve(str->length() + length);

	static auto handle = [](void* ptr, const byte_t* data, length_t len) {
		std::string* str = (std::string*)ptr;
		str->append(data, len);
		return len;
	};
	return dispatch(handle, str);
}

length_t
BaseBuffer::write_string(void const* string)
{
    std::string* str = (std::string*)string;
    length_t length = str->length();
	write_int32(length);
	write(str->c_str(), length);
	return length + sizeof(length_t);
}

}
