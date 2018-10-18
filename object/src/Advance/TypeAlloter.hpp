
#pragma once

#include "Advance/WrapAlloter.hpp"

namespace common
{
	/** 
	 * @brief type alloc interface
	 * @note work as alloter wrapper for type, automately 
	 * load construct and destruct
	 */
	template<class Type>
	class TypeAlloter : public WrapAlloter
	{
	public:
		/** 
		 * @brief base construct, use sys-alloc or other alloter
		 */
		TypeAlloter(int64_t limit = c_length_1K, bool lock = true, BaseAlloter* alloter = NULL)
			: WrapAlloter(alloter ? alloter : this, limit, lock) { BaseAlloter::set(sizeof(Type), c_length_align); }

		/**
		 * must set clear here, for no inherit called here
		 **/
		virtual ~TypeAlloter() {}

	public:
		/**
		 * get next type
		 **/
		Type*		get() { return (Type*)_new(); }

		/**
		 * put type
		 **/
		void		put(void* type) { _del((Type*)type); }

		/**
		 * @brief new for request len
		 * @param len mem len
		 */
		virtual void* _new(size_t len = 0) {
			void* data = WrapAlloter::_new();
			return ::new (data) Type;
		}

		/**
		 * @brief del unused
		 * @param buffer del buffer address
		 * @param len del len, unused most times
		 */
		virtual void  _del(void* data, size_t len = 0) {
			((Type*)data)->~Type();
			WrapAlloter::_del(data, len);
		}
	};
}
#if COMMON_SPACE
	using common::TypeAlloter;
#endif


