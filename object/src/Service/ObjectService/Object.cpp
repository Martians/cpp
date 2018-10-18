
#define LOG_CODE 0

#include "Common/Display.hpp"
#include "Advance/Pointer.hpp"
#include "Advance/FastHash.hpp"
#include "Advance/DynChunk.hpp"
#include "UnitTest/Mock.hpp"
#include "ObjectService/LogWriter.hpp"
#include "ObjectService/Object.hpp"

using namespace common;

const int Object::Head::s_magic[] = {
	0x000000000L, (int)0x8C9864E0DL, (int)0x912ACD805L, 0x00000000L
};

Object*
Object::Malloc()
{
	return new Object;
}

void
Object::Cycle()
{
    delete this;
}

int32_t
Object::Head::EndPos()
{
	assert(page_align(length) == actual);
	return actual + object::c_object_head_size;
}

int32_t
Object::Head::CheckPos()
{
	return EndPos() - c_page_size;
}

int32_t
Object::Head::CheckSize()
{
	return c_page_size;
}

uint32_t
Object::Head::HeadHash()
{
	return ::SuperFastHash(Start(), c_head_fixed_size - sizeof(hash))
		+ ::SuperFastHash(keyptr, keylen);
}

uint32_t
Object::Head::LastHash(char* data)
{
	uint32_t block_len = std::min(c_length_4K, length);
	if (!data) {
		data = object->mData.tail()->wpos() - c_page_size;
	}
	return ::SuperFastHash(data, block_len);
}

void
Object::Head::Write(char* data, uint32_t total, int32_t off)
{
	length = object->mLength;
	actual = object->mActual;
	offset = off;

	char* keep = data;
	keylen = strlen(object->mKey.name);
	keyptr = object->mKey.name;

	hash[0] = HeadHash();
	hash[1] = LastHash();

	write_data(data, Start(), c_head_fixed_size);
	write_data(data, keyptr, keylen);

	assert(data - keep <= total);
}

int
Object::Head::Read(char* data, uint32_t total)
{
	memcpy(Start(), data, c_head_fixed_size);
	keyptr = data + c_head_fixed_size;

	uint32_t value = HeadHash();
	if (value != hash[0]) {
		log_info("object read head, " << StringObject((*this)) << ", but head hash "
			<< value << ", not match recorded " << hash[0]);
		return -1;
	}
	return 0;
}

int
Object::Head::Last(char* data, uint32_t total)
{
	uint32_t value = LastHash(data);

	if (value != hash[1]) {
		log_info("object read last, " << StringObject((*this)) << ", but block hash "
			<< value << " not match recorded " << hash[1]);
		return -1;
	}
	return 0;
}

void
Object::Ajustment()
{
	static char data[c_page_size] = {0};
    DynBuffer::Chunk* chunk = mData.tail();
	uint32_t len = page_align(chunk->length())
		- chunk->length();

	if (len != 0) {
		mData.write(data, len);
		assert(chunk == mData.tail());
	}
	mActual = mData.length();
}

int
Object::OccupySize()
{
	/** if come here, buffer already fill last */
    return mActual + object::c_object_head_size;
}

std::string
UnitPath(const UnitIndex& index)
{
	char data[4096] = {};
	char curr[64] = {};

	int64_t current = index.index;
	snprintf(curr, sizeof(curr), "/%lld", (long_int)current);
	current /= c_object_dir_files;

	do {
		int64_t level = current % object::c_object_dir_width;
		current /= object::c_object_dir_width;

		snprintf(data, sizeof(data), "/%02lld%s", (long_int)level, curr);
		memcpy(curr, data, strlen(data));
	} while (current > 0);
	return data;
}

#if 0
	void
	path_test()
	{
		for (int i = 0; i < 100000; i++) {
			log_debug(UnitPath((UnitIndex)i));
		}
		exit(0);
	}
#endif
