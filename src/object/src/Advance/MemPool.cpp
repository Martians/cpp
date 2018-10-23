
#define LOG_CODE 0

#include <sstream>

#include "Common/Const.hpp"
#include "Common/LogHelper.hpp"
#include "Common/Display.hpp"
#include "Common/Time.hpp"
#include "Common/Util.hpp"
#include "Common/CodeHelper.hpp"
#include "Advance/MemPool.hpp"
#include "Advance/Pointer.hpp"
#include "Advance/SingleList.hpp"

namespace common {

using namespace memory;

uint32_t
MemConfig::align_len(uint32_t align) {
	int length = memory::c_align;
	return std::max(1 << up_exp(align), length);
}

uint32_t
MemConfig::unit_len(uint32_t unit) {
	return unit == 0 ? memory::c_unit_len
		: up_align(unit, memory::c_boundary);
}

/**
 * memory alloc unit
 *
 * @brief b0|head|b2|data|tail|head|b2|data|tail|b1|.....
 * b0 is the first piece blank for mem alloc
 * tail is used for head align and valid, validate length for each unit
 **/
struct MemUnit : public BaseAlloter
{
public:
	MemUnit(const MemConfig& config);

	virtual ~MemUnit() {
		reset_array(m_data);
	}

public:

	/**
	 * @brief piece head
	 * @note total struct must align to machine word long
	 */
	struct Head {
		/** piece head magic */
		uint32_t	magic = { 0 };
		/** piece head tag */
		uint32_t	tag	= { 0 };
		/** next piece */
		SingleLink	link;
	};

	/**
	 * variable length tail
	 **/
	struct Tail {
		byte_t		data[memory::c_tail_max] = {0};
	};

public:
	/**
	 * alloc new piece
	 **/
	virtual void* _new(size_t len = 0) {
		if (m_list.empty()) {
			return NULL;
		}
		Head* head = (Head*)m_list.deque();
		/** keep unit pointer */
		head->link.set(this);
		return piece_data(head);
	}

	/**
	 * @brief del unused
	 * @param chunk del chunk address
	 * @param len del len, unused most times
	 */
	virtual void  _del(void* data, size_t len = 0) {
		Head* head = piece_head(data);
		if (check_memory()) {
			assert(check_head(head));
			assert(check_tail(head));
		}
		head->link.clear();
		m_list.enque(head);
	}

public:
	/**
	 * memory unit length
	 **/
	uint32_t utlen() { return m_config.utlen; }

	/**
	 * get align
	 **/
	uint32_t align() { return m_config.align; }

	/**
	 * total capacity
	 **/
	uint32_t capacity() { return m_capacity; }

	/**
	 * remain chunk count
	 **/
	uint32_t size() { return m_list.size(); }

	/**
	 * list is empty
	 **/
	bool	emtpy() { return m_list.empty(); }

	/**
	 * list is full, all item exist
	 **/
	bool	full() { return m_list.size() == m_capacity; }

	/**
	 * check if memory unit is valid
	 **/
	bool	check();

	/**
	 * update timer
	 **/
	void	update_time() { m_time.reset(); }

	/**
	 * check if local is timeout
	 **/
	bool	timeout() { return m_time.expired(); }

	/**
	 * dump prefix string
	 **/
	const std::string& string() { return m_config.display; }

	/** piece head length */
	static const int c_head_len = sizeof(Head);
	/** static tail for validate */
	static const uint64_t c_tail[memory::c_tail_max / sizeof(uint64_t)];

public:
	/**
	 * check algin when initialize
	 * */
	static void check_align(void* ptr, uint32_t align) {
		uint64_t start = (uint64_t)ptr;
		uint64_t mode  = up_align(start, align);
		assert(start == mode);
	}

	/**
	 * check for valid
	 **/
	static bool check_head(Head* head) {
		return head->magic == memory::c_magic;
	}

	/**
	 * get data by head
	 **/
	static byte_t* piece_data(Head* head) { return (byte_t*)head + c_head_len; }

	/**
	 * get head by data
	 **/
	static Head* piece_head(const void* data) { return (Head*)((byte_t*)data - c_head_len); }

	/**
	 * get unit from data
	 **/
	static MemUnit* unit(const void* ptr) {
		Head* head = piece_head(ptr);
		if (check_head(head)) {
			MemUnit* unit = (MemUnit*)head->link.next;
			if (unit->valid()) {
				return unit;
			}
		}
		return NULL;
	}

	/**
	 * check if local is valid
	 **/
	bool	valid() { return m_magic == memory::c_magic; }

protected:
	/**
	 * calculate inner state
	 **/
	void	calculate();

	/**
	 * format memory unit display
	 **/
	void	format_display();

	/**
	 * dump inner state
	 **/
	std::string dump(bool origin);

	/**
	 * check if head valid
	 **/
	bool	valid(Head* head) {
		return check_head(head) && check_tail(head);
	}

	/**
	 * set tail origin data
	 **/
	void 	set_tail(Head* head) {
		byte_t* ptr = ((byte_t*)head + m_step - m_tail);
		memcpy(ptr, &c_tail, m_tail);
	}

	/**
	 * check if tail valid
	 **/
	bool	check_tail(Head* head) {
		byte_t* ptr = ((byte_t*)head + m_step - m_tail);
		return memcmp(ptr, &c_tail, m_tail) == 0;
	}

public:
	/** memory config */
	MemConfig m_config;
	/** used for valid */
	uint32_t m_magic  = { memory::c_magic };
	/** link in pool */
	SingleList m_list = { OFFSET(Head, link) };
	/** piece or container data start */
	byte_t*  m_data	 = { NULL };
	/** actual data start */
	byte_t*  m_start = { NULL };
	/** piece step */
	uint32_t m_step  = { 0 };
	/** tail length */
	uint32_t m_tail  = { 0 };
	/** origin item count */
	uint32_t m_capacity = { 0 };
	/** local timer */
	TimeCheck m_time;
	/** link in memory pool */
	ListLink m_link;
};

const uint64_t MemUnit::c_tail[] = {
	0x599783649B9EAB47LL, 0xD7498D06B0985490LL,
	0x98902345A8793347LL, 0xC7498D06B0985490LL
};

uint32_t
mem_piece(const void* ptr)
{
	return MemUnit::unit(ptr)->piece();
}

MemUnit::MemUnit(const MemConfig& config)
	: BaseAlloter(config.piece), m_config(config)
{
	calculate();

	m_time.set(m_config.timeout);
	/** align head start, b0 = data - ptr */
	//posix_memalign((void**)&data, getpagesize(), len);
	byte_t* ptr = malloc_align(m_data, utlen(), align());
	byte_t* end = m_data + utlen() + align();

	while (ptr + m_step <= end) {
		Head* head = new(ptr) Head;
		head->magic = memory::c_magic;
		set_tail(head);
		m_list.enque(head);

		check_align(head, c_align);
		check_align(piece_data(head), align());
		ptr = ptr + m_step;
	}
	assert(m_capacity == m_list.size());
	format_display();

	if (m_config.ct_size != 0) {
		trace(string() << dump(true));
	}
}

std::string
MemUnit::dump(bool origin)
{
	std::stringstream ss;
	if (origin) {
		ss << " align " << align() << " tail " << m_tail << " step " << m_step << ", ";
	}
	ss << "length " << string_size(utlen()) << ", size " << size() << "/" << capacity();
	return ss.str();
}

void
MemConfig::calculate(uint32_t& step, uint32_t& tail)
{
	/** fixed-length step length */
	step = MemUnit::c_head_len + piece;
	tail = up_align(step, align) - step;
	/** need keep some tail char, larger than tail_min, still need align */
	if (tail < memory::c_tail_min) {
		tail = up_align(step + memory::c_tail_min, align) - step;
	}
	step += tail;
}

void
MemUnit::calculate()
{
	/** align and boundary must be 2-exp based */
	assert(exp_base(align()));
	assert(exp_base(memory::c_boundary));
	/** tail part make the next piece aligned */
	//assert(memory::c_tail_max >= align());

	/** if set count, reset unit length */
	if (m_config.ct_size != 0) {
		/** get actual data count, alloc data with no head or tail */
		uint32_t count = m_config.utlen / m_config.piece;

		/** fix config for container */
		m_config.piece = m_config.ct_size;
		m_config.align = c_align;

		m_config.calculate(m_step, m_tail);
		m_config.utlen = MemConfig::unit_len((count + 1) * m_step);

	} else {
		m_config.calculate(m_step, m_tail);
	}
	m_capacity = utlen() / m_step;
}

void
MemUnit::format_display()
{
	/** format dispaly string */
    format(m_config.display, "unit %d-%s",
		m_config.index, string_size(piece(), false).c_str());
}

bool
MemUnit::check()
{
	/** align head start, b0 = data - ptr */
	byte_t* ptr = address_align(m_data, align());

	size_t count = 0;
	while (++count <= m_capacity) {
		Head* head = (Head*)ptr;
		assert(valid(head));

		check_align(head, c_align);
		check_align(piece_data(head), align());
		ptr = ptr + m_step;
	}
	return true;
}

/**
 * chunk unit, use origin memory for container, alloc additional data buffer
 **/
class ContainUnit : public MemUnit
{
public:
	struct Contain {
		byte_t* data = {NULL};
	};

	ContainUnit(const MemConfig& config) : MemUnit(config) {
		/** refresh piece length and string */
		m_config = config;
		piece(config.piece);
		format_display();

		byte_t* ptr = malloc_align(m_buffer, m_config.utlen, align());
	    byte_t* end = m_buffer + utlen() + align();

	    uint32_t count = (end - ptr) / piece();
	    assert(capacity() >= count);
	    reduce(capacity() - count);

	    count = 0;
	    Contain* type = NULL;
		m_list.init();
		while (ptr + piece() <= end) {
			Head* head = (Head*)m_list.next();
			type = (Contain*)(piece_data(head) + m_config.ct_off);
			assert(type != NULL);

			type->data = ptr;
			ptr += piece();
			count++;
		}

		assert(capacity() == count);
		trace(string() << dump(true));
	}

	virtual ~ContainUnit() {
		reset_array(m_buffer);
	}

protected:
	/**
	 * reduce item, while container count larger than data count
	 **/
	void	reduce(uint32_t count) {
		m_capacity -= count;
		while (count-- > 0) {
			m_list.deque();
		}
	}

public:
	/** chunk start */
	byte_t*	m_buffer = { NULL };
};

MemPool::MemPool(const MemConfig& config)
	: BaseAlloter(config.piece), m_mutex("mem pool"), m_config(config),
	  m_free(OFFSET(MemUnit, m_link)), m_used(OFFSET(MemUnit, m_link)), m_full(OFFSET(MemUnit, m_link))
{
	regist_alloter(this);
}

void
cycle_chunk_unit(void* item)
{
	MemUnit* unit = (MemUnit*)item;
	delete unit;
}

MemPool::~MemPool()
{
    regist_alloter(this, false);
	Mutex::Locker lock(m_mutex);

	m_free.clear(cycle_chunk_unit);
	m_used.clear(cycle_chunk_unit);
	m_full.clear(cycle_chunk_unit);
}

MemUnit*
MemPool::pop_unit()
{
	MemUnit* unit = NULL;
	if (!m_used.empty()) {
		unit = (MemUnit*)m_used.tailv();
		if (unit->size() == 1) {
			m_used.del(unit);
			m_full.enque_tail(unit);
			trace(unit->string() << " up to full");
		}

	} else if (!m_free.empty()) {
		unit = (MemUnit*)m_free.deque_tail();
		m_used.enque_tail(unit);
		trace(unit->string() << " up to used");

	} else {
		if (m_config.limit != 0) {
			size_t size = m_used.size() + m_free.size() + m_full.size();
			if (size * m_config.utlen > m_config.limit) {
				log_info("alloc memory exceed limit " << string_size(size * m_config.utlen));
				return NULL;
			}
		}
		if (m_config.ct_size != 0) {
			unit = new ContainUnit(m_config);
		} else {
			unit = new MemUnit(m_config);
		}
		m_config.index++;
		m_used.enque_head(unit);
	}
	return unit;
}

void
MemPool::push_unit(MemUnit* unit)
{
	if (unit->size() == 1) {
		m_full.del(unit);
		m_used.enque_tail(unit);
		trace(unit->string() << " down to used");

	} else if (unit->full()) {
		m_used.del(unit);
		m_free.enque_tail(unit);
		unit->update_time();
		trace(unit->string() << " down to free");
	}
}

void*
MemPool::new_piece(size_t len)
{
	MemUnit* unit = pop_unit();
	if (!unit) {
		return NULL;
	}
	return unit->_new(len);
}

void
MemPool::del_piece(void* data, size_t len)
{
	MemUnit* unit = MemUnit::unit(data);
	assert(unit->valid());
	unit->_del(data);
	push_unit(unit);
}

void
MemPool::release()
{
	Mutex::Locker lock(m_mutex);
	MemUnit* unit = NULL;
	m_free.init();

	while ((unit = (MemUnit*)m_free.next())) {
		if (unit->timeout()) {
			trace("memory release, cycle " << unit->string() << ", remain " << m_free.size() - 1);

			m_free.del(unit);
			delete unit;
		}
	}
	trace("memory status, empty " << m_full.size() << ", used " << m_used.size() << ", free " << m_free.size());
}

}

#include <vector>
#include <algorithm>
#include "Common/ThreadBase.hpp"

namespace common {
class RegistAlloter : public ThreadBase
{
public:
	RegistAlloter() : ThreadBase(5000) {
		assert(start() == 0);
	}

	virtual ~RegistAlloter() { stop(); }

public:
	/**
	 * regist new alloter
	 **/
	void	regist_alloter(BaseAlloter* alloter, bool regist) {
		Mutex::Locker lock(mutex());
		if (regist) {
			m_array.push_back(alloter);
		} else {
			m_array.erase(std::remove(m_array.begin(), m_array.end(), alloter));
		}
	}

	/**
	 * release alloter
	 **/
	void	release_alloter() {
		Mutex::Locker lock(mutex());
		for (auto alloter : m_array) {
			alloter->release();
		}
	}

protected:
	/**
	 * thread work
	 **/
    virtual void *entry() {

		while (waiting()) {
			release_alloter();
		}
		return NULL;
	}

protected:
	std::vector<BaseAlloter*> m_array;
};

SINGLETON(RegistAlloter, alloter_checker);

void
regist_alloter(BaseAlloter* alloter, bool regist)
{
	alloter_checker().regist_alloter(alloter, regist);
}

}

#if COMMON_TEST
#include "Perform/TestUtil.hpp"
#include "Perform/Debug.hpp"

namespace common {
    MemConfig& chunk_config(MemConfig& config);

namespace tester {
	
	void
	mem_pool_test()
	{
	    MemConfig config1(57, 16, c_length_1K - 52);
		MemPool s_pool1(chunk_config(config1));

		MemConfig config2(c_length_4K, c_length_4K, c_length_64K);
		MemPool s_pool2(chunk_config(config2));

		MemConfig config3(57, 16, c_length_1K);
		MemPool s_pool3(chunk_config(config3));

		check_memory(true);
		set_random();

		batchs(10, alloter_test, &s_pool1, 10000, 500);
		batchs(10, alloter_test, &s_pool2, 1000, 50);
		batchs(10, alloter_test, &s_pool3, 10000, 500);
		thread_wait();
	}
}
}

#endif
