
#pragma once

#include <string>

#include "Common/Define.hpp"
#include "Common/Mutex.hpp"
#include "Advance/List.hpp"
#include "Advance/BaseAlloter.hpp"

namespace common {

	namespace memory {
		/** default piece align boundary */
		static const int c_align     = c_length_align;
        /** piece tail length */
        static const int c_tail_max  = 32;
        /** piece tail length */
        static const int c_tail_min  = 8;
        /** default piece length */
        static const int c_piece_len = 64 * c_length_1K;
        /** the boundary for alloc large piece of memory */
        static const int c_boundary	 = 4 * c_length_1K;
        /** default memory unit length */
        static const int c_unit_len	 = 8 * c_length_1M;
		/** memory unit magic */
		static const int64_t c_magic = 0xFF56879LL;
        /** unit timeout for free */
        static const int c_timeout  = 10000;
	}

	/**
	 * check memory or not
	 **/
	inline bool check_memory(bool set = -1) {
		static bool s_check = false;
		if (set != -1) {
			s_check = set;
		}
		return s_check;
	}

	/**
	 * memory config
	 **/
	struct MemConfig
	{
	public:
		MemConfig(uint32_t _pilen = 0, uint32_t _align = 0, uint32_t _utlen = 0, uint64_t _limit = 0) {
			set(_pilen, _align, _utlen, _limit);
		}

		MemConfig(const MemConfig& v) {
			operator = (v);
		}

		const MemConfig& operator = (const MemConfig& v) {
			timeout	= v.timeout;

			piece = v.piece;
			align = v.align;
			utlen = v.utlen;
			limit = v.limit;
            index = v.index;

            ct_size = v.ct_size;
            ct_off = v.ct_off;
			return *this;
		}

	public:
		/**
		 * set inner config
		 **/
		void		set(uint32_t _pilen = 0, uint32_t _align = 0, uint32_t _utlen = 0, uint64_t _limit = 0) {
			piece = piece_len(_pilen);
			align = align_len(_align);
			utlen = unit_len (_utlen);
			limit = _limit;
		}

		/**
		 * set contain info
		 **/
		void		contain(uint32_t piece, uint32_t offset) {
			ct_size = piece;
			ct_off = offset;
		}

		/**
		 * get align len by default
		 **/
		static uint32_t align_len(uint32_t align);

		/**
		 * get unit len by default
		 **/
		static uint32_t unit_len(uint32_t unit);

		/**
		 * get piece len by default
		 **/
		static uint32_t piece_len(uint32_t piece) { return piece == 0 ? memory::c_piece_len : piece; }

		/**
		 * calculate step and tail length
		 **/
        void 	calculate(uint32_t& step, uint32_t& tail);

	public:
		/** display string */
		std::string display;
		/** timeout for free */
		uint32_t timeout = { memory::c_timeout };
		/** piece length */
		uint32_t piece = { 0 };
		/** default align */
		uint32_t align = { 0 };
		/** memory unit length */
		uint32_t utlen = { 0 };
		/** memory pool limit */
		uint64_t limit = { 0 };
		/** unit index */
		uint64_t index = { 0 };
		/** memory unit for container size */
		uint32_t ct_size = { 0 };
		/** memory unit for container position */
		uint32_t ct_off = { 0 };
	};

    class MemUnit;
	/**
	 * chunk pool
	 **/
	class MemPool : public BaseAlloter
	{
	public:
		MemPool(const MemConfig& config);

		virtual ~MemPool();

	public:
		/**
		 * alloc new chunk
		 *
		 * @note if exceed limit will failed
		 **/
		virtual void* _new(size_t len = 0) {
			Mutex::Locker lock(m_mutex);
			return new_piece(len);
		}

		/**
		 * @brief del unused
		 * @param data chunk address
		 * @param len unused most times
		 */
		virtual void  _del(void* data, size_t len = 0) {
			Mutex::Locker lock(m_mutex);
			del_piece(data, len);
		}

		/**
		 * release resource timely
		 **/
		virtual void release();

		/**
		 * get config
		 **/
		MemConfig*	config() { return &m_config; }

	protected:
		/**
		 * pop a unit with chunk
		 **/
		MemUnit* pop_unit();

		/**
		 * push a chunk back to unit
		 **/
		void 	push_unit(MemUnit* unit);

		/**
		 * alloc new piece
		 **/
		void*	new_piece(size_t len = 0);

		/**
		 * del piece
		 **/
		void	del_piece(void* data, size_t len = 0);

	protected:
		Mutex		m_mutex;
		/** unit config */
		MemConfig 	m_config;
		/** empty list */
		List		m_free;
		/** half list */
		List		m_used;
		/** full list */
		List		m_full;
	};

	/**
	 * get unit piece from data
	 **/
	uint32_t mem_piece(const void* ptr);

	/**
	 * add alloter to memory cycle
	 **/
	void	regist_alloter(BaseAlloter* alloter, bool regist = true);
}

