
#pragma once

#include <functional>

#include "Common/Cond.hpp"
#include "Common/Define.hpp"
#include "Common/Container.hpp"
#include "Advance/AppHelper.hpp"

namespace common {

	/**
	 * coordinate event notify
	 **/
	class Coordinator
	{
	public:
		Coordinator() {}

		struct Event {
		public:
			Event(int _type, typeid_t _unique, int _total = 0)
				: type(_type), unique(_unique), total(_total) {}

		public:
			/**
			 * check if event is complete
			 **/
			bool	completed() { return count >= total; }

			/**
			 * inc wait count
			 **/
			bool	inc() {
				count++;
				return completed();
			}

			/**
			 * get remain count
			 **/
			int		remain() {
				return count >= total ? 0 : total - count;
			}

		public:
			/** source index */
			int		index	= {0};
			/** event type */
			int 	type	= {0};
			/** event unique */
			typeid_t unique = {0};
			/** total count */
			int		total	= {0};
			/** done count */
			int		count	= {0};
		};
	public:
		/**
		 * set total count
		 **/
		void	total(int total) { m_total = total; }

		/**
		 * regist wait event
		 **/
		typeid_t regist_single(int type, int count = 0) {
			Mutex::Locker lock(m_mutex);

			Event temp(type, m_unique.next(),
				count = 0 ? m_total : count);
			success(m_event.add(temp));
			return temp.unique;
		}

		/**
		 * regist wait event array
		 **/
		void	regist_array(int beg, int end) {
			Mutex::Locker lock(m_mutex);

			for (int type = beg; type < end; type++) {
				Event temp(type, m_unique.next(), m_total);
				success(m_event.add(temp));
			}
		}

		/**
		 * get event unique, event must be unique in map
		 **/
		typeid_t unique(int type) {
			Mutex::Locker lock(m_mutex);

			Event* event = get(type, 0);
			return !event ? -1 : event->unique;
		}

		/**
		 * get event remain count
		 **/
		int		remain(int type, typeid_t unique = 0) {
			Event* event = get(type, unique);
			return event ? event->remain() : -1;
		}

	public:
		/**
		 * notify and wait
		 **/
		bool	notify_wait(typeid_t index, int type, typeid_t unique = 0) {
			if (notify(index, type, unique)) {
				return true;
			}
			wait(type, unique);
			return false;
		}

		/**
		 * notify self and wait event happen
		 **/
		bool	notify(typeid_t index, int type, typeid_t unique = 0) {
			Mutex::Locker lock(m_mutex);
			Event* event = get(type, unique);
			if (event) {
				if (event->inc()) {
					m_cond.signal_all();
					return true;
				}
			}
			return false;
		}

		/**
		 * wait until event happen
		 **/
		void	wait(int type, typeid_t unique = 0) {
			Mutex::Locker lock(m_mutex);
			while (!completed(type, unique)) {
				m_cond.wait(m_mutex);
			}
		}

	protected:
		/**
		 * get inner event
		 **/
		Event*  get(int type, typeid_t unique = 0) {
			Event event(type, unique);
			return m_event.get(event);
		}

		/**
		 * check if event complete
		 **/
		bool	completed(int type, typeid_t unique = 0) {
			Event* event = get(type, unique);
			return event && event->completed();
		}
	protected:
		Mutex	m_mutex  = {"coordinator", true};
		Cond	m_cond;
		Unique<> m_unique;
		int		m_total	 = {0};
		/** event already done */
		TypeSet<Event> m_event;
	};

	inline bool
	operator < (const Coordinator::Event& a, const Coordinator::Event& b) {
		//return std::tie(a.type, a.unique) < std::tie(b.type, b.unique);
		/** equal */
		if (a.type == 0 || b.type == 0) {
			return false;

		} else if (a.type < b.type) {
			return true;

		} else if (a.type > b.type) {
			return false;
		}
		/** equal */
		if (a.unique == 0 || b.unique == 0) {
			return false;

		} else {
			return a.unique < b.unique;
		}
	}

	class WaitEvent
	{
	public:
		WaitEvent() {}

		enum {
			WE_fail = -2,
			WE_none = -1,
			WE_wake = 0,
			WE_wait = 1000000,
			WE_done = 2000000,
		};
		typedef std::function<void()> handle_t;

		/**
		 * local event
		 **/
		struct Event {
			Event(int c = 0, handle_t h = handle_t())
				: count(c), handle(h) {}

			int		count = {1};
			handle_t handle;
		};

	public:
		/**
		 * clear local info
		 **/
		void 	reset() {
			m_unique = 0;
			m_count	= 0;
			m_fail = 0;

			m_cond.reset();
			m_type.clear();
			m_unexpect.clear();

			handle_t handle;
			m_wake.handle.swap(handle);
		}

		/**
		 * regist wait type
		 *
		 * @note defualt set event count as 1
		 **/
		void 	regist(int type, typeid_t unique = 0) {
			Mutex::Locker lock(m_lock);
			success(m_type.add(type, 1));
			m_unique = unique;
		}

		/**
		 * regist wait array
		 **/
		void 	regist(std::initializer_list<int> array, typeid_t unique = 0) {
			Mutex::Locker lock(m_lock);
			for (const auto& type : array) {
				success(m_type.add(type, 1));
			}
			m_unique = unique;
		}

		/**
		 * set event count, only count match will trigger wakeup
		 **/
		void	regist_count(int type, int count, typeid_t unique = 0) {
			Mutex::Locker lock(m_lock);
			Event *event = get_event(type, unique);
			if (!event) {
				assert(0);

			/** regist event, but count is 0, no need wait */
			} else if ((event->count = count) == 0) {
				m_count++;
			}
		}

		/**
		 * set event count, only count match will trigger wakeup
		 **/
		void	regist_handle(int type, const handle_t& handle) {
			Mutex::Locker lock(m_lock);
			Event *event = get_event(type);
			if (!event) {
				event = m_type.add(type, 1);
			}
			event->handle = handle;
			event->count = std::max(1, event->count);
		}

		/**
		 * set event count, only count match will trigger wakeup
		 **/
		void	regist_handle(const handle_t& handle) {
			Mutex::Locker lock(m_lock);
			m_wake.handle = handle;
		}

		/**
		 * regist wait type
		 *
		 * @note defualt set event count as 1
		 **/
		void 	regist_unexpect(int type) {
			Mutex::Locker lock(m_lock);
			success(m_unexpect.add(type, 0));
		}
	
		/**
		 * regist wait array
		 **/
		void 	regist_unexpect(std::initializer_list<int> array) {
			Mutex::Locker lock(m_lock);
			for (const auto& type : array) {
				success(m_unexpect.add(type, 0));
			}
		}

		/**
		 * cancel the event
		 **/
		void	cancel(int type, typeid_t unique = 0) {
			Mutex::Locker lock(m_lock);
			m_type.del(type);
		}

		/**
		 * check if event is regist
		 **/
		bool	registed(int type, typeid_t unique = 0) {
			Mutex::Locker lock(m_lock);
			return get_event(type, unique) != NULL;
		}

		/**
		 * check if event is done
		 **/
		bool	event_done(int type, typeid_t unique = 0) {
			Mutex::Locker lock(m_lock);
			Event *event = get_event(type, unique);
			if (event) {
				return get_status(event) == WE_done;
			} else {
				return false;
			}
		}

		/**
		 * get mutex
		 **/
		Mutex&	mutex() { return m_lock; }

		/**
		 * initial unhappen event
		 **/
		bool	init_wait(bool unexpect = false) {
			return unexpect ? m_unexpect.init() : m_type.init();
		}

		/**
		 * get next unhappen event
		 **/
		int		next_wait(int& key, bool unexpect = false) {
			int ret = 0;
			while (1) {
				Event* event = unexpect ? m_unexpect.next(&key)
					: m_type.next(&key);
				if (event) {
					/** event not done */
					if (get_status(event) != WE_done) {
						ret = event->count;
						break;
					}
				} else {
					ret = -1;
					break;
				}
			}
			return ret;
		}

	public:
		/**
		 * wait until event happen
		 **/
		int 	wait(uint32_t time = 20) {
			Mutex::Locker lock(m_lock);
			int ret = 0;

			if (result(ret)) {
				m_cond.reset();
				return ret;
			}
			ret = m_cond.wait_interval(m_lock, time);

			result(ret);
			return ret;
		}

		/**
		 * event happen, try to wakeup
		 *
		 * @return 0 means all event done
		 * @return 1 means event reduce to done or not, and still remain some event
		 * @return-1 means event not regist, or already reduce to 0
		 **/
		int 	wakeup(int type, typeid_t unique = 0) {
			Mutex::Locker lock(m_lock);
			int ret = WE_none;
			Event *event = NULL;
			if ((event = get_event(type, unique))) {
				ret = set_status(event);
				if (ret == WE_wake) {
					trigger(event);

					if (done()) {
						m_cond.signal_all();
						trigger(&m_wake);
						return WE_wake;
					}
					ret = WE_wait;

				} else if (ret == WE_done) {
					ret = WE_wait;
				}

			} else {
				event = m_unexpect.get(type);
				if (event) {
					event->count++;
					trigger(event);

					m_fail++;
					m_cond.signal_all();
					return WE_fail;

				} else if (done()) {
					ret = WE_done;
				}
			}
			return ret;
		}

	protected:
		/**
		 * get event count pointer
		 **/
		Event*	get_event(int type, typeid_t unique = 0) {
			if (m_unique == 0 || m_unique == unique) {
				return m_type.get(type);
			}
			return NULL;
		}

		/**
		 * get type status
		 **/
		int		get_status(Event* event) {
			return event->count == 0 ? WE_done : WE_wait;
		}

		/**
		 * set_count event count
		 *
		 * @return 0 means event trigger
		 * @return 1 means event reduce, not to 0;
		 * @return-1 means event not regist, or already reduce to 0
		 **/
		int		set_status(Event* event) {
			if (event->count == 0) {
				return WE_done;

			} else {
				if (--(event->count) == 0) {
					m_count++;
					return WE_wake;

				} else {
					return WE_wait;
				}
			}
		}

		/**
		 * trigger event
		 **/
		void	trigger(Event* event) {
			if (event->handle) {
				m_lock.unlock();
				event->handle();
				m_lock.lock();
			}
		}

		/**
		 * check if all done
		 **/
		bool	done() { return (m_type.size() == 0 || m_type.size() == m_count); }

		/**
		 * check if failed
		 **/
		bool	fail() { return m_fail != 0; }

		/**
		 * check if get result
		 **/
		bool	result(int& ret) {
			if (fail()) {
				ret = WE_fail;

			} else if (done()) {
				ret = WE_wake;

			} else {
				return false;
			}
			return true;
		}

	protected:
		Mutex 	m_lock = {"wait event"};
		Cond 	m_cond;
		typeid_t m_unique = {0};
		/** regist event set_count */
		TypeMap<int, Event> m_type;
		TypeMap<int, Event> m_unexpect;
		Event	m_wake;
		/** complete count */
		size_t	m_count = {0};
		size_t	m_fail  = {0};
	};

}
#if COMMON_SPACE
	using common::WaitEvent;
#endif

