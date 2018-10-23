
#pragma once

#include "Common/Cond.hpp"
#include "Common/Atomic.hpp"

namespace common {

	#define	BARRIER_CHECK	1

	/**
	 * barrier base
	 **/
	class Barrier
	{
	public:
		Barrier(const char* name = NULL)
			: m_mutex(name == NULL ? "barrier" : name) {}

		virtual ~Barrier() {}

	public:
		/**
		 * enter into barrier, block with any put_up
		 **/
		virtual void enter() = 0;

		/**
		 * exit barrier, unblock any put_up
		 **/
		virtual void exit() = 0;

		/**
		 * put_up barrier, block all put_up and enter
		 **/
		virtual void put_up() = 0;

		/**
		 * take down barrier, unblock any put_up and enter
		 **/
		virtual void take_down() = 0;

		/**
		 * check current state
		 **/
		virtual void check(bool block) = 0;

	public:
		/**
		* scope enter
		**/
		class Enter {
		public:
			Enter(Barrier& br, bool lock = true) : barrier(br) {
			   if (lock) {
				   block();
				}
			}
			~Enter() {
				unblock();
			}
		public:
			void block() {
				if (!lock) {
					lock = true;
					barrier.enter();
					check();
				}
			}
			void unblock() {
				if (lock) {
					check();
					lock = false;
					barrier.exit();
				}
			}
			void check() {
			#if BARRIER_CHECK
				barrier.check(false);
			#endif
			}
		protected:
			Barrier& barrier;
			bool lock = {false};
		};

	   /**
		* scope blocker
		**/
		class Block {
		public:
			Block(Barrier& br, bool lock = true) : barrier(br) {
				if (lock) {
					block();
				}
			}
			~Block() {
				unblock();
			}
		public:
			void block() {
				if (!lock) {
					lock = true;
					barrier.put_up();
					check();
				}
			}
			void unblock() {
				if (lock) {
					check();
					lock = false;
					barrier.take_down();
				}
			}
			void check() {
			#if BARRIER_CHECK
				barrier.check(true);
			#endif
			}
		protected:
			Barrier& barrier;
			bool lock = {false};
		};
	protected:
		/** lock mutex */
		Mutex   m_mutex;
		/** block condition */
		Cond    m_block_cond;
		/** enter condition */
		Cond    m_enter_cond;
		/** reference count */
		volatile int m_count = {0};
		/** already block or not */
		volatile bool m_block = {false};

		/** waiting readers */
		int		m_reader = {0};
		/** waiting writers */
		int		m_writer = {0};
	};

	/**
	 * barrier for enter and block
	 **/
	class BaseBarrier : public Barrier
	{
	public:
		BaseBarrier(const char* name = NULL)
			: Barrier(name) {}
	public:
		/**
		 * enter into barrier, block with any put_up
		 **/
		virtual void enter() {
			Mutex::Locker lock(m_mutex);
			++m_reader;
			while (m_block) {
				m_block_cond.wait(m_mutex);
			}
			--m_reader;

			++m_count;
		}

		/**
		 * exit barrier, unblock any put_up
		 **/
		virtual void exit() {
			Mutex::Locker lock(m_mutex);
			assert(m_count > 0);
			if (--m_count == 0 && m_block) {
				m_enter_cond.signal_one();
			}
		}

		/**
		 * put_up barrier, block all put_up and enter
		 **/
		virtual void put_up() {
			Mutex::Locker lock(m_mutex);

			++m_writer;
			/** wait other put up end */
			while (m_block) {
				m_block_cond.wait(m_mutex);
			}
			--m_writer;

			m_block = true;
			/** wait enter end */
			while (m_count != 0) {
				m_enter_cond.wait(m_mutex);
			}
		}

		/**
		 * take down barrier, unblock any put_up and enter
		 **/
		virtual void take_down() {
			Mutex::Locker lock(m_mutex);
			m_block = false;
			if (m_reader > 0) {
				m_block_cond.signal_all();

			} else if (m_writer > 0) {
				m_block_cond.signal_one();
			}
		}

		/**
		 * check current state
		 **/
		virtual void check(bool block) {
			if (block) {
				assert(m_count == 0 && m_block == true);

			} else {
				assert(m_count > 0);
			}
		}

	};

	/**
	 * fast lock for rwlock
	 *
	 * @note reader first
	 **/
	class FastBarrier : public Barrier
	{
	public:
		FastBarrier(const char* name = NULL)
			: Barrier(name) {}

	public:
		/**
		 * reader wait until no wlock
		 *
		 * @note if writer is work, count is negative, when writer complete,
		 *   it will known reader is waiting
		 * @note reader first here, if reader coming, the other writer can't get
		 *   lock again, for m_count is not 0
		 **/
		virtual void enter() {
			if (atomic_add(&m_count, 1) < 0) {

				/** old count < 0, writer already exist, wait */
				Mutex::Locker lock(m_mutex);
				while (m_count < 0) {
					m_block_cond.wait(m_mutex);
				}
			}
		}

		/**
		 * reader unlock, wakeup if no writer
		 *
		 * @note if writer wait before reader unlock, then m_wait must > 0,
		 *   for we inc m_wait before writer wait
		 * @note if writer wait after reader unlock, if m_wait is 0, then writer have not
		 *   try atomic_comp_swap yet, it will get the lock; if m_wait > 0 now, then reader will do wakeup
		 **/
		virtual void exit() {
			assert(m_count > 0);
			/** no reader exist now */
			if (atomic_add(&m_count, -1) == 1) {

				/** writer is waiting */
				if (m_writer > 0) {
					Mutex::Locker lock(m_mutex);
					/** notify one writer */
					m_block_cond.signal_one();
				}
			}
		}

		/**
		 * write lock
		 * @note count > 0, rlock enter; count < 0, wlock enter
		 **/
		virtual void put_up() {

			Mutex::Locker lock(m_mutex);
			++m_writer;
			while (atomic_comp_swap(&m_count, c_wlock, 0) != 0) {
				m_block_cond.wait(m_mutex);
			}
			--m_writer;
		}

		/**
		 * write unlock
		 **/
		virtual void take_down() {
			assert (m_count < 0);

			Mutex::Locker lock(m_mutex);
			/* old value larger than c_wlock, means reader is waiting,
			 * should signal_all to wakeup all readers; if writer is
			 * waiting, should notify too */
			if (atomic_add(&m_count, -c_wlock) > c_wlock) {
				m_block_cond.signal_all();

			} else if (m_writer > 0) {
				m_block_cond.signal_one();
			}
		}

		/**
		 * check current state
		 **/
		virtual void check(bool block) {
			if (block) {
				assert(m_count < 0 && m_count >= c_wlock);
			} else {
				assert(m_count > 0);
			}
		}

		/** defualt lock */
		static const int c_wlock = -100000000;
	};
}
