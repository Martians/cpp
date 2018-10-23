
#include <cstdlib>
#include <memory>

#include "Common/TypeQueue.hpp"
#include "Common/Container.hpp"
#include "Advance/Util.hpp"
#include "Advance/BaseAlloter.hpp"

namespace common {

/**
 * alloter for local thread
 **/
class ThreadAlloter
{
public:
	ThreadAlloter() { assert(c_local_limit >= c_local_batch); }

	~ThreadAlloter() {
		for (BatchAlloter& ba : m_array) {
			ba.cycle();
		}
	}

public:
	/**
	 * set local thread alloter config
	 **/
	void 		set(BaseAlloter* alloter, int type, uint32_t limit = 0, uint32_t batch = 0) {
		assert(type < c_max_size);
		m_array[type].set(alloter, limit, batch);
	}

	/**
	 * get local thread alloter
	 **/
	BaseAlloter* get(int type) {
		return &m_array[type];
	}

public:
	class BatchAlloter : public BaseAlloter {
	public:
		BatchAlloter() { set(); }
		virtual ~BatchAlloter() { cycle(); }

	public:
		/**
		 * set alloter status
		 **/
		void	set(BaseAlloter* alloter = NULL, uint32_t limit = 0, uint32_t batch = 0) {
			m_alloter = alloter;
			m_limit = (limit == 0 ? c_local_limit : limit);
			m_batch = (batch == 0 ? c_local_batch : batch);

            if (m_alloter) {
			    piece(m_alloter->piece());
            }
		}
		/**
		 * @brief new for request len
		 * @param len mem len
		 */
		virtual void* _new(size_t len = 0) {
			if (m_list.size() == 0) {
				/** alloc failed */
				if (!apply_for(len)) {
					return NULL;
				}
			}
			void* data = m_list.deque_back();
			return data;
		}

		/**
		 * @brief del unused
		 * @param buffer del buffer address
		 * @param len del len, unused most times
		 */
		virtual void  _del(void* data, size_t len = 0) {
			m_list.enque(data);
			put_back();
		}

		/**
		 * release inner resource
		 **/
		virtual void cycle() {
            if (m_alloter) {
            	release();
			    m_alloter->cycle();
                m_alloter = NULL;
            }
		}

		/**
		 * release inner resource
		 **/
		virtual void release() {
			for (auto data: m_list) {
				m_alloter->_del(data);
			}
			m_list.clear();
		}

	protected:
		/**
		 * apply for more piece
		 **/
		bool 	apply_for(size_t len) {
			int count = 0;
			while (count++ < m_batch) {
				void* data = m_alloter->_new(len);
				if (data) {
					m_list.enque(data);
				} else {
					break;
				}
			}
			return m_list.size() > 0;
		}

		/**
		 * put back some piece
		 **/
		void 	put_back() {
			if ((int)m_list.size() > m_limit + m_batch) {
				while ((int)m_list.size() > m_limit) {
					void* data = m_list.deque_back();
					m_alloter->_del(data);
				}
			}
		}

	protected:
		/** local keep piece */
		TypeQueue<void*> m_list;
		/** origin alloter */
		BaseAlloter*	 m_alloter = { NULL };
	    int     m_limit = { 0 };
		int 	m_batch = { 0 };
	};

	/** max local BaseAlloter count */
	static const int c_max_size = 16;
	/** local piece keep limit */
	static const int c_local_limit = 16;
	/**  */
	static const int c_local_batch = 16;

protected:
	BatchAlloter m_array[c_max_size];
};

thread_local std::shared_ptr<ThreadAlloter> s_local;

void*
BaseAlloter::_new(size_t len)
{
	if (m_align == 0) {
		return ::malloc(up_align(piece(), c_malloc_unit));

	} else {
		return aligned_alloc(m_align, m_piece);
	}
}

void
BaseAlloter::_del(void* data, size_t len)
{
	::free(data);
}

void
LocalAlloter(BaseAlloter* alloter, int type, int limit, int batch)
{
	if (!s_local) {
		s_local = std::make_shared<ThreadAlloter>();
	}
	s_local->set(alloter, type, limit, batch);
}

BaseAlloter*
LocalAlloter(int type)
{
	return s_local->get(type);
}
}

#if COMMON_TEST

#include "Perform/TestUtil.hpp"
#include "Perform/Debug.hpp"
using namespace common;

namespace common {
namespace tester {
    void
	__local_alloter(BaseAlloter* alloter, int type, int m_limit, int m_batch) {
    	LocalAlloter(alloter, type, m_limit, m_batch);
    	alloter_test(LocalAlloter(type), 10000, 300);
	}

	void
	base_alloter_test()
	{
		BaseAlloter base(37);
		set_random();

		batchs(30, __local_alloter, &base, 0, 10, 5);
		thread_wait();
	}
}
}
#endif

