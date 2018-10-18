
#pragma once

#include "Common/Container.hpp"
#include "Common/TypeQueue.hpp"
#include "Common/ThreadInfo.hpp"
#include "Advance/BaseAlloter.hpp"
#include "Advance/BaseBuffer.hpp"

namespace common
{
    class Barrier;
};
namespace common {
namespace tester {

class AlloterUtil
{
public:
	AlloterUtil(BaseAlloter* at, int64_t _limit, int64_t _hold) {
		m_alloter = at;
		m_limit = _limit;
		m_hold 	= _hold;
	}
	virtual ~AlloterUtil() { clear(); }

public:
	/**
	 * memory test work
	 **/
	void	work();

protected:

	/**
	 * alloc or malloc
	 **/
	bool 	do_alloc() { return random() % 7 < 5; }

	/**
	 * work from head or tail
	 **/
	bool 	do_head() { return random() % 3 == 0; }

	/**
	 * do enque work
	 **/
	void	enque() {
		void* data = m_alloter->_new();
		if (do_head()) {
			m_list.enque_front(&data);
		} else {
			m_list.enque(&data);
		}
		m_stat_new++;
	}

	/**
	 * do deque work
	 **/
	void	deque() {
		void* data = do_head() ?
			m_list.deque() : m_list.deque_back();
		m_alloter->_del(data);
		m_stat_del++;
	}

	/**
	 * clear all data
	 **/
	void	clear() {
		/** release chunk */
		loop_for_each_ptr(void, data, m_list) {
			m_alloter->_del(data);
		}
        m_list.clear();
	}

public:
	/** total work count */
	int64_t		m_limit;
	/** max hold limit */
	int64_t 	m_hold;
	/** mock mem using */
	TypeQueue<void*> m_list;
	BaseAlloter* 	m_alloter;
	int64_t		m_stat_new = {0};
	int64_t		m_stat_del = {0};
};

inline void
alloter_test(BaseAlloter* alloter, int64_t limit, int64_t hold)
{
	AlloterUtil test(alloter, limit, hold);
	test.work();
}

#if 0
class BufferUtil
{
public:
	BufferUtil(BaseBuffer* buffer, int64_t count, int64_t limit = c_int64_max) {
		m_buffer = buffer;
		m_limit = std::min(m_limit, (int64_t)c_length_1M);
		m_count = count;
	}
	virtual ~BufferUtil() { clear(); }

public:
	/**
	 * memory test work
	 **/
	void	work() {
		write_full();
		write_fix();
	}

	void	write_full() {
		while (write(::random() % 100));
		while (read(17));
		assert(m_buffer->length() == 0);
	}

	void	write_fix() {
		while (write(7));
		m_buffer->remove();

		while (write(17));
		m_buffer->remove();

		while (write(57));
		while (read(7));
		assert(m_buffer->length() == 0);
	}

	/**
	 * write and check
	 **/
	void	write_check() {

	}

protected:
	/**
	 *
	 **/
	bool	read(int size) {
		if (m_length < size) {
			size = m_length;
			if (size == 0) {
				return false;
			}
		}
		assert(m_buffer->read(m_data, size) == size);
		return true;
	}

	bool	write(int size) {
		if (m_length + size > m_limit) {
			size = m_limit - m_length;
			if (size == 0) {
				return false;
			}
		}
		assert(m_buffer->write(m_data, size) == size);
		return true;
	}

public:
	char		m_data[c_length_64K];
	/** total work count */
	int64_t		m_limit = {0};
	/** max hold limit */
	int64_t 	m_count = {0};
	/** current length */
	int64_t		m_length = {0};
	/** current buffer */
	BaseBuffer* m_buffer = {NULL};
};

inline void
buffer_test(BaseBuffer* buffer, int64_t count, int64_t limit)
{
	BufferUtil test(buffer, count, limit);
	test.work();
}
#endif

/**
 * check lock performance
 **/
class BarrierUtil
{
public:
	BarrierUtil(Barrier* barrier, bool block, int64_t limit)
		: m_barrier(barrier), m_block(block), m_limit(limit) {}
public:
	/**
	 * do test work
	 **/
	void	work();

protected:
	/**
	 * loop work to consume time
	 **/
	void	loop() {
		int64_t total = 0;
		for (int64_t i = 0; i < m_limit; i++) {
			total += i;
		}
	}
protected:
	Barrier* m_barrier = {NULL};
	/** block or enter */
	bool	m_block = {false};
	/** total work count */
	int64_t	m_limit;
};
}
}
using namespace common;
using namespace common::tester;



