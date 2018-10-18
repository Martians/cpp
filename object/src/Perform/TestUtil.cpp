
#include <unistd.h>
#include <iostream>

#include "Common/Logger.hpp"
#include "Common/Display.hpp"
#include "Advance/Barrier.hpp"
#include "Perform/Debug.hpp"
#include "Perform/Timer.hpp"
#include "Perform/TestUtil.hpp"

namespace common {
namespace tester {

void
AlloterUtil::work()
{
	/** alloc and clear */
	int64_t count = m_limit;
	while (count-- >= 0) {
		enque();
	}
	clear();

	count = m_limit;
	while (--count >= 0) {
		if (m_list.size() >= (size_t)m_hold) {
			int count = ::random() % m_hold;
			while (count-- > 0) {
				deque();
			}
		}
		/** alloc data to queue */
		if (do_alloc() || (int64_t)m_list.size() < m_hold /2) {
			enque();

		/** release data */
		} else {
			deque();
		}
		//log_debug("count " << list.size());
	}
	clear();
	log_debug("piece " << string_size(m_alloter->piece())
		<< ", total new " << m_stat_new << " del " << m_stat_del);
}

void
BarrierUtil::work()
{
    const char* mode = m_block ? "block" : "enter";

    static int s_block = -1;
    static int s_enter = -1;
    int index = m_block ? atomic_add(&s_block, 1)
             : atomic_add(&s_enter, 1);

	log_info("barrier work, mode " << mode << " index " << index + 1);
	for (int64_t i = 0; i < m_limit; i++) {
		if (m_block) {
			Barrier::Block block(*m_barrier);
			loop();

		} else {
			Barrier::Enter enter(*m_barrier);
			loop();
		}
	}
}

inline void
__barrier_test(Barrier* barrier, int block_fraction, int64_t limit)
{
	bool block = (thread_info()->m_index * 10) % 100 < block_fraction;
	BarrierUtil test(barrier, block, limit);
	test.work();
}

::common::BaseBarrier base;
::common::FastBarrier fast;

void barrier_test() {

	int thread = 50;
	int block_faction = 50;
	int limit = 5000;

	CREATE_TIMER;

	set_random();
	batchs(thread, __barrier_test, &fast, block_faction, limit);
    THREAD_TIMER("fast barrier");
	
    set_random();
	batchs(thread, __barrier_test, &base, block_faction, limit);
	THREAD_TIMER("base barrier");
}

}
}
