
#pragma once

#include "Common/Define.hpp"

namespace common
{
	/** length align, min alloc length */
	const int c_malloc_unit = 8;

	/** 
	 * @brief base mem alloc interface
	 */
	class BaseAlloter
	{
	public:
		BaseAlloter(size_t pilen = 0, size_t align = 0, bool owner = false)
			: m_piece(pilen), m_align(align), m_owner(owner) {}
		virtual ~BaseAlloter() {}

	public:
		/** 
		 * @brief new for request len
		 * @param len mem len
		 */
		virtual void* _new(size_t len = 0);

		/** 
		 * @brief del unused
		 * @param buffer del buffer address
		 * @param len del len, unused most times
		 */
		virtual void  _del(void* data, size_t len = 0);

		/**
		 * release resource timely
		 **/
		virtual void release() {}

		/**
		 * @brief cycle itself
		 */
		virtual void cycle() {
			if (owner()) {
				release();
				delete this;
			}
		}
	public:
		/**
		 * set params
		 **/
		void 	set(size_t pilen = 0, size_t align = 0, bool owner = false) {
			m_piece = pilen;
			m_align = align;
			m_owner = owner;
		}

		/**
 		 * @brief set piece length
		 */
		void	piece(size_t pilen) { m_piece = pilen; }

		/**
 		 * @brief get piece length
		 */
		size_t	piece() { return m_piece; }

		/**
		 * set owner
		 **/
		void	owner(bool set) { m_owner = set; }

		/**
		 * get owner
		 **/
		bool	owner() { return m_owner; }

	protected:
		/** piece length */
		size_t	m_piece = {0};
		/** align length */
		size_t	m_align = {0};
		/** user owner alloter or not */
		bool	m_owner = {false};
	};

	/**
	 * set local alloter
	 **/
	void	LocalAlloter(BaseAlloter* alloter, int type, int limit = 0, int batch = 0);

	/**
	 * get local alloter
	 **/
	BaseAlloter* LocalAlloter(int type);
}

