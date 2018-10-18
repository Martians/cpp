
#pragma once

#include "Common/Const.hpp"

namespace common
{
	class BaseArray
	{
	public:
		BaseArray(size_t size = 0) { reset(size); }

		~BaseArray() { clear(true); }

		/**
		 * clear inner data
		 **/
		void	clear(bool cycle = false) {
			if (cycle) {
				reset_array(m_array);
				m_capacity = 0;
			} else {
				memset(m_data, 0, m_capacity * sizeof(void*));
			}
			m_head = 0;
			m_tail = 0;
			m_size = 0;
		}

	public:
		/**
		 * set array capacity
		 **/
		void	reset(size_t capacity = 0) {
			if (capacity == 0) {
				clear(true);
				return;
			}

			Type* data = m_data;
			m_data = new byte_t[capacity];
			memset(m_data, 0, capacity * sizeof(Type));

			if (m_head <= m_tail) {
				memcpy(m_data, m_head, m_size * sizeof(Type));
			} else {
				size_t left = (m_capacity - m_head) * sizeof(Type);
				memcpy(m_data, data + m_head, left);
				memcpy(m_data + left, data, m_tail * sizeof(Type));
			}
			m_capacity = capacity;

			if (data != m_fixed) {
				reset_array(data);
			}
		}
		/**
		 * get array size
		 **/
		size_t	size() { return m_size; }

		/**
		 * get array capacity
		 **/
		size_t	capacity() { return m_capacity; }

		/**
		 * put value
		 **/
		void	put(void* data) {
			extend();

			*inc() = data;
		}

		void*   pop() {

			if (!shrink()) {
				return NULL;
			}
			return dec();
		}

	protected:
		/**
		 * base Type
		 **/
		//struct Type {
		//	void* data = {NULL};
		//};
		typedef byte_t Type;

		void	fix_pos(size_t& pos) {
			if (pos >= m_capacity) {
				pos %= m_capacity;
			}
		}

		/**
		 * move head to next
		 **/
		Type* 	dec() {
			assert(m_size > 0);
			size_t pos = m_head;
			--m_size;
			++m_head;
			fix_pos(m_head);
			return m_data[pos];
		}

		/**
		 * move tail to next
		 **/
		Type* 	inc() {
			assert(m_size < m_capacity);
			size_t pos = m_tail;

			++m_size;
			++m_tail;
			fix_pos(m_tail);
			return m_data[pos];
		}

		Type*	head() { return m_data[m_head]; }
		Type*	tail() { return m_data[m_tail]; }

		/**
		 * get Type
		 **/
		Type* operator [] (int pos) {
			if (m_head + pos > m_tail) {
				return NULL;
			}
			return m_data[m_head + pos];
		}




		/**
		 * extend size
		 **/
		void	extend() {
			if (m_size >= m_capacity) {
				reset(m_capacity << 1);
			}
		}

		void	shrink() {
			if (m_size < (m_capacity >> 1)) {
				reset(m_capacity >> 1);
			}
		}

		const int c_minimize = 2;

	protected:
		size_t	m_size = {0};
		Type*	m_head = NULL;
		Type* 	m_tail = NULL;
		Type*	m_data = {NULL};
		size_t	m_capacity = {0};
	};
}

