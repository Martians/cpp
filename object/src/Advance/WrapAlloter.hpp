
#pragma once

#include "Common/Const.hpp"
#include "Common/Mutex.hpp"
#include "Common/Define.hpp"
#include "Common/TypeQueue.hpp"
#include "Advance/BaseAlloter.hpp"

namespace common
{
	/** 
	 * @brief wrapper alloc interface
	 * @note work as alloter wrapper for lock and queue
	 */
	class WrapAlloter : public BaseAlloter
	{
	public:
		/** 
		 * @brief base construct, use sys-alloc or other alloter
		 */
		WrapAlloter(BaseAlloter* alloter, int limit = c_length_1K, bool lock = false)
			: m_alloter(alloter), m_limit(limit), m_lock(lock) {}

		virtual ~WrapAlloter() {
			clear();
		}

	public:
		/**
		 * @brief set new alloter
		 */
		void		set(BaseAlloter* alloter) { m_alloter = alloter; }

		/**
		 * get current size
		 **/
		int			size() { return m_list.size(); }

		/**
		 * clear inner
		 **/
		void		clear() {
			for (auto data : m_list) {
				cycle(data);
			}
			m_list.clear();
		}

	public:
		/** 
		 * @brief new for request len
		 * @param len mem len
		 */
		virtual void* _new(size_t len = 0) {
			Mutex::Locker lock(m_mutex, Locking());
			void* data = m_list.deque();
			if (!data) {
				data = malloc(len);
			}
            return data;
		}

		/** 
		 * @brief del unused
		 * @param buffer del buffer address
		 * @param len del len, unused most times
		 */
		virtual void  _del(void* data, size_t len = 0) {

			Mutex::Locker lock(m_mutex, Locking());
			if (m_list.size() < m_limit) {
				m_list.enque(data);
			} else {
				cycle(data, len);
			}
		}

	protected:
		/**
		 * need lock or note
		 **/
		bool 	Locking() { return m_lock; }

		/**
		 * malloc new piece
		 **/
		void*	malloc(size_t len = 0) {
			if (m_alloter == this) {
				return BaseAlloter::_new(len);

			} else {
				return m_alloter->_new(len);
			}
		}

		/**
		 * cycle piece
		 **/
		void	cycle(void* data, size_t len = 0) {
			if (m_alloter == this) {
				return BaseAlloter::_del(data);

			} else {
				return m_alloter->_del(data, len);
			}
		}

	protected:
		/** object mutex */
		Mutex 	m_mutex = { "type alloter" };
		/** wrapper alloter */
		BaseAlloter* m_alloter = {NULL};
		/** total limit */
		size_t	m_limit = {0};
		/** use lock or note */
		bool  	m_lock 	= {false};
		/** temparary list */
		TypeQueue<void*> m_list;
	};
}

#if COMMON_SPACE
	using common::WrapAlloter;
#endif

