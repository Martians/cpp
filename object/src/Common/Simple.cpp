
#include "Common/Type.hpp"
#include "Common/Const.hpp"
#include "Common/Global.hpp"
#include "Common/Mutex.hpp"
#include "Common/CodeHelper.hpp"
#include "Common/TypeQueue.hpp"

const void* c_invalid_ptr = NULL;

namespace common {

	/**
	 * @brief get local nbo type
	 */
	inline bool local_nbo() {
		union {
			int  x;
			char y;
		} z;
		z.x = 1;
		return z.y != 1;
	}

	const int c_local_bod = local_nbo();

	class GlobalHandle
	{
	public:
		GlobalHandle() {
		}

		~GlobalHandle() {
			trigger(false);
		}
	public:
		/**
		 * regist handle
		 **/
		void	regist(void_handle_t handle, bool init) {
			Mutex::Locker lock(m_mutex);
			Queue& que = init ? m_init : m_exit;
			que.enque(handle);
		}

		/**
		 * trigger handle
		 **/
		void	trigger(bool init) {
			Mutex::Locker lock(m_mutex);
			Queue& que = init ? m_init : m_exit;
			for (auto handle: que) {
				handle();
			}
			que.clear();
		}
	protected:
		typedef TypeQueue<void_handle_t> Queue;
		Mutex	m_mutex = {"global handle"};
		Queue	m_init;
		Queue	m_exit;
	};
	SINGLETON(GlobalHandle, GetGlobal);

	void
	regist_init(void_handle_t handle) {
		GetGlobal().regist(handle, true);
	}

	void
	regist_exit(void_handle_t handle) {
		GetGlobal().regist(handle, false);
	}

	void
	initialize() {
		GetGlobal().trigger(true);
	}

	void
	exit() {
		GetGlobal().trigger(false);
	}

}


