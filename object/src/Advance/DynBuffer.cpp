
#include <cstring>

#include "Advance/DynBuffer.hpp"
#include "Advance/DynChunk.hpp"
#include "Advance/MemPool.hpp"

namespace common {
/** fix memconfig for dynchunk */
MemConfig& chunk_config(MemConfig& config) {
	typedef DynBuffer::Chunk Chunk;
	config.contain(sizeof(Chunk), OFFSET(Chunk, data));
	return config;
}

static BaseAlloter*
dyn_chunk_pool() {
	static MemConfig s_config(DynBuffer::c_length);
	static MemPool s_dyn_chunk_pool(chunk_config(s_config));
	return &s_dyn_chunk_pool;
}

void
dyn_chunk_pool(length_t piece, length_t align) {
	MemPool* pool = (MemPool*)dyn_chunk_pool();
	pool->config()->set(piece, align);
}

DynBuffer::Chunk*
DynBuffer::Chunk::_new(void* data)
{
	byte_t* ptr = ((Chunk*)data)->data;
	return ::new(data) Chunk(ptr);
}

length_t
DynBuffer::Chunk::total() const
{
	return mem_piece(this);
}

void
DynBuffer::Chunk::assign(Chunk* chunk, length_t len)
{
	success(total() >= chunk->length());
	len = std::min(len, chunk->length());
	memcpy(data, chunk->rpos(), len);
	beg = 0;
	end	= len;
}

void
DynBuffer::Chunk::adjust()
{
	length_t len = length();
	memmove(data, data + beg, len);
	beg = 0;
	end = len;
}

DynBuffer::Chunk*
DynBuffer::append_chunk(Chunk* chunk)
{
	if (!chunk && !(chunk = new_chunk())) {
		return NULL;
	}

	if (!m_head) {
		m_head = chunk;
	} else {
		m_tail->next = chunk;
	}
	m_tail = chunk;
	inc(chunk->length());
	return chunk;
}

length_t
DynBuffer::dispatch(data_handle_t handle, void* ptr, length_t len, length_t off) const
{
	Chunk* cur = m_head;
	while (off && cur) {
		if (cur->length() > off) {
			break;
		}
		off -= cur->length();
		cur = cur->next;
	}
	//off maybe remain sth, it will set to 0 in the first loop
	length_t pos = 0;
	length_t ret = 0;
	while (pos < len && cur) {
		length_t rd = std::min(cur->length() - off, len - pos);
		ret = handle(ptr, cur->rpos() + off, rd);
		if (ret < 0) {
			return ret;
		}

		pos += rd;
		off = 0;
		cur = cur->next;
	}
	return pos;
}

length_t
DynBuffer::write(const void* data, length_t len)
{
	Chunk* cur = m_tail;
	length_t pos = 0;
	while (pos < len) {
		if (!cur && !(cur = append_chunk())) {
			//fault_format("can't alloc more buffer ");
			assert(0);
			return 0;
		}
		length_t wt = std::min(len - pos, cur->remain());
		memcpy(cur->wpos(), (const char*)data + pos, wt);
		cur->inc(wt);
		pos += wt;
		cur = cur->next;
	}
	inc(pos);
	return pos;
}

length_t
DynBuffer::remove(length_t len)
{
	Chunk* cur = m_head;
	length_t pos = 0;
	while (len && cur) {
		if (cur->length() > len) {
			cur->dec(len);
			pos += len;
			break;
		}
		Chunk* next = cur->next;
		pos += cur->length();
		len -= cur->length();
		del_chunk(cur);
		cur = next;
	}
	m_head = cur;
	if (!m_head) {
		m_tail = NULL;
	}
	dec(pos);
	return pos;
}

length_t
DynBuffer::retrive(length_t rlen)
{
	/** should not retrive more than data length */
	if (rlen >= length()) {
		rlen = length();
		clear();
		return rlen;
	}

	/** get remain length, len must > 0 and < data_len */
	length_t len = length() - rlen;
	length_t pos = len;
	Chunk* cur = m_head, *next = m_head;

	while (len && cur) {
		if (cur->length() < len) {
			next = cur;
			len -= cur->length();
			cur  = cur->next;
		} else {
			len = cur->length() - len;
			cur->retrieve(len);

			/** no data remain, cur should be delete */
			if (cur->length() == 0) {
				m_tail = next;

			/** some data remain, cur should be kept */
			} else {
				m_tail = cur;
				cur = cur->next;
			}
			m_tail->next = NULL;
			break;
		}
	}
	while (cur) {
		Chunk* next = cur->next;
		del_chunk(cur);
		cur = next;
	}
	length(pos);
	return rlen;
}

BaseAlloter*
DynBuffer::alloter()
{
	return m_alloter ? m_alloter
		: dyn_chunk_pool();
}

DynBuffer::Chunk*
DynBuffer::new_chunk()
{
	Chunk* chunk = (Chunk*)alloter()->_new();
	return Chunk::_new(chunk);
}

void
DynBuffer::del_chunk(Chunk* chunk)
{
	alloter()->_del(chunk);
}

void
DynBuffer::clear()
{
	Chunk* cur = m_head;
	while (cur) {
		Chunk* next = cur->next;
		del_chunk(cur);
		cur = next;
	}
	m_head = m_tail = NULL;
	m_length = 0;
}

void
DynBuffer::adjust()
{
	if (m_head) {
		m_head->adjust();
	}
}

char 
DynBuffer::operator[] (length_t pos) const
{
	Chunk* cur = m_head;
	while (cur) {
		if (cur->length() > pos) {
			return *(cur->rpos() + pos);
		}
		pos -= cur->length();
		cur  = cur->next;
	}
	return -1;
}

//length_t
//DynBuffer::copy(DynBuffer* dyn, length_t len)
//{
//	len = min(dyn->data_len(), len);
//	length_t pos = 0;
//	//if new add data can be insert into the last chunk
//	if (m_end && len <= m_end->remain()) {
//		Chunk* cur = dyn->m_start;
//		while (pos < len && cur) {
//			length_t wt = min(cur->data_len(), len - pos);
//			memcpy(m_end->write_pos(), cur->read_pos(), wt);
//			pos += wt;
//			m_end->inc(wt);
//		}
//	} else {
//		Chunk* cur = dyn->m_start;
//		while (pos < len && cur) {
//			Chunk* chunk = append_chunk();
//			chunk->assign(cur, min(len - pos, cur->data_len()));
//			pos += chunk->data_len();
//			cur = cur->next;
//		}
//	}
//	inc(pos);
//	return pos;
//}


length_t
DynBuffer::append(DynBuffer* dyn, length_t len)
{
	len = std::min(dyn->length(), len);
	if (len == dyn->length()) {
		if (!m_head) {
			m_head	= dyn->m_head;
		} else {
			m_tail->next = dyn->m_head;
		}
		m_tail = dyn->m_tail;
		inc(dyn->length());

		length_t ret = dyn->length();
		dyn->m_head = dyn->m_tail = NULL;
		dyn->m_length = 0;
		return ret;

	} else {
		length_t pos = 0;
		Chunk* cur = dyn->m_head, *last = NULL;
		while (pos < len && cur) {
			if (pos + cur->length() > len) {
				break;
			}
			last = cur;
			pos += cur->length();
			cur = cur->next;
		}

		//append whole chunks to us
		if (last) {
			if (!m_head) {
				m_head = dyn->m_head;
			} else {
				m_tail->next = dyn->m_head;
			}
			dyn->m_head = cur;
			dyn->dec(pos);

			m_tail = last;
			m_tail->next = NULL;
		}

		//append tail
		if (pos < len) {
			len = len - pos;
			Chunk* chunk = append_chunk();
			chunk->assign(cur, len);
			cur->dec(len);
			dyn->dec(len);
			pos += len;
		}
		if (!dyn->m_head) {
			dyn->m_tail = NULL;
		}
		inc(pos);
		return pos;
	}
}	

void	
DynBuffer::check()
{
	length_t len = 0;
	Chunk* cur = m_head;
	while (cur) {
		success(cur->beg <= cur->end);
		success(cur->end <= cur->total());
		len += cur->length();
		cur = cur->next;
	}
	success(len == length());
}
}

#if COMMON_TEST

#include <sstream>
#include <unistd.h>
#include "Common/Logger.hpp"
#include "Common/Display.hpp"
#include "Advance/BufferStream.hpp"

using namespace common;
using std::string;

void
DynBuffer::dump()
{
    int count = 0;
    Chunk* cur = m_head;
    while (cur) {
        std::stringstream ss;
        ss << "<" << count++ << std::endl;
        for (length_t i = 0; i < cur->length(); i++) {
            ss << (*cur)[i];
        }
        log_debug(ss.str());
        cur = cur->next;
    }
    check();
}

namespace common {
namespace tester {


	void
	dyn_buffer_test()
	{
		MemConfig config(128, 64);
		MemPool dyn_pool(chunk_config(config));

		//DynBuffer::start();
		DynBuffer ba(&dyn_pool), bb(&dyn_pool);
		int count = 0;

		char array[8] = {0};
		char table[] = {"12345678910,11,12,1122334455667788"};
		ba.write(table, 7);
		ba.write(table + 7, 4);
		ba.write(table + 11, 6);
		ba.write(table + 17, 10);

		char ch = 0;
		ch = ba[1];
		ch = ba[9];
		ch = ba[25];

		ba.peek(&array, 8);
		ba.peek(&array, 8, 5);
		ba.peek(&array, 8, 11);
		ba.peek(&array, 8, 15);

		bb.write(&ch, 1);
		bb.copy(&ba, 2);
		bb.append(&ba, ba.length() -1);

		bb.clear();
		ba.clear();
		ba.write(table, 7);

		bb.append(&ba, 2);

		ba.remove(1);
		ba.peek(&array, 4);
		ba.clear();
		ba.remove(-1);
		length_t n = 37, n1;
		byte_t   c = 39, c1;
		string s = "abcdefghijklmnopqrstuvwxyz", s1;

		ba << n;
		ba << c;
		ba << s;
		ba.dump();

		ba >> n1;
		ba >> c1;
		ba >> s1;

		ba.dump();

		ba << n;
		ba << c;
		ba << s;
		bb.copy(&ba);
		bb.dump();
		bb.append(&ba, ba.length() -1);
		bb.dump();
		bb.clear();

		char tmp[c_length_1K * 1];
		count = 10000;
		ba.clear();
		while (count -- > 0) {
			if (ba.write(tmp, sizeof(tmp)) == 0) {
				ba.clear();
				usleep(100000);
			}
			if (ba.length() >= c_length_1M * 3) {
				ba.clear();
			}
		}
		ba.clear();
	}
}
}
#endif



