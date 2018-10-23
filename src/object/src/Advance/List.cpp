
#include <new>

#include "Common/Util.hpp"
#include "Advance/List.hpp"
#include "Advance/BaseAlloter.hpp"

namespace common
{

void
List::clear(item_destory_t destroy)
{
	if (destroy || m_alloter) {
		ListLink* ptr;
		init();
		while ((ptr = next_link())) {
			void* item = del_link(ptr);
			if (destroy) {
				destroy(item);
			}
		}
	}
	m_head.init_head();
	m_size = 0;
}

void
List::set_bit(int flag, bool set)
{
	common::set_bit(m_bits, flag, set);
}

bool
List::get_bit(int flag)
{
	return common::get_bit(m_bits, flag);
}

ListLink*
List::new_link(void* ptr)
{
	if (m_alloter) {
		DataLink* link = (DataLink*)m_alloter->_new();
		::new(link) DataLink(ptr);
		return link;
	} else {
		return link(ptr);
	}
}

void*
List::del_link(ListLink* link)
{
	if (m_alloter) {
		void* data = ((DataLink*)link)->data;
		m_alloter->_del(link);
		return data;
	} else {
		return entry(link);
	}
}

void
List::append(List* v)
{
	/** must check this, should not link v.head */
	if (v->empty()) return;

	v->m_head.prev->next = &m_head;
	v->m_head.next->prev = m_head.prev;

	m_head.prev->next = v->m_head.next;
	m_head.prev = v->m_head.prev;

	m_size += v->m_size;

	/** be empty */
	v->m_head.init_head();
	v->m_size = 0;

	assert(m_pos == v->m_pos &&
		m_alloter == v->m_alloter);
}

void
List::swap(List* v)
{
	/** must check this, should not link v.head */
	if (v->empty()) return;

	v->m_head.prev->next = &m_head;
	v->m_head.next->prev = m_head.prev;

	m_head.prev->next = v->m_head.next;
	m_head.prev = v->m_head.prev;

	m_size += v->m_size;

	/** be empty */
	v->m_head.init_head();
	v->m_size = 0;

	assert(m_pos == v->m_pos &&
		m_alloter == v->m_alloter);
}

void
HighLink::del()
{
	list->del(this);
}

}
