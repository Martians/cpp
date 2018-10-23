
#pragma once

#define LOG_CODE_HPP 0

#include "Common/Common.hpp"
#include "Common/Container.hpp"
#include "Common/CodeHelper.hpp"
#include "Advance/Coordinator.hpp"
#include "Perform/LogHelper.hpp"

namespace mock {
	/** default mock wait time */
	const int c_mock_wait 	= 2000;
	/** invalid int number */
	const int c_invalid_int	= 100000000;

	const int SERVER_1 = 1;
	const int SERVER_2 = 2;
	const int SERVER_3 = 3;
	const int SERVER_4 = 4;
	const int SERVER_5 = 5;

	/** host not exist */
	extern const char* c_not_exist_host;
	/** host exist but port not listen */
	const int16_t c_not_listen_port = 1001;
	/** string not exist */
	extern const char* c_not_exist_value;
	/** duplicate value */
	extern const char* c_duplicate_value;
}
using namespace mock;

class BaseMock
{
public:
	BaseMock() {
        trace("base mock create");
	}

public:
	/**
	 * reset inner status
	 **/
	void		Reset() {
		mStrMap.clear();
		mDataMap.clear();
		mWaitEvent.reset();
		mSyncType = 0;
	}

	/**
	 * set default wait time
	 **/
	void		SetWaitTime(int time = mock::c_mock_wait) { mWaitTime = time; }

	/**
	 * get wait time
	 **/
	int			WaitTime() { return mWaitTime; }

	/**
	 * dump event if wait timeout
	 **/
	void		SetDumpEvent(bool set = true) { mDumpWait = set; }

	/**
	 * set step state
	 **/
	void		SetStep(bool set) { mStep = set; }

public:
	/**
	 * do step work
	 **/
	void		StepWait();

	/**
	 * get next id
	 **/
	uint64_t	NextID() { return mAllocer++; }

	/**
	 * set type err
	 **/
	void		String(int type, const std::string& str) {
		success(mStrMap.add(type, str));
	}

	/**
	 * get type error
	 **/
	const std::string& String(int type) {
		std::string *pstr = mStrMap.get(type);
		const std::string& str = pstr ? *pstr : "";
		//m_str_map.del(type);
		return str;
	}

	/**
	 * set type data
	 **/
	void		SetData(int type, int data = 1) {
		mDataMap.del(type);
		mDataMap.add(type, &data);
	}

	/**
	 * get type data
	 **/
	int			Data(int type) {
		int* pdata = mDataMap.get(type);
		int data = pdata ? *pdata : -1;
		//mDataMap.Del(type);
		return data;
	}

	int			DecData(int type) {
		int data = -1;
		int* pdata = mDataMap.get(type);
		if (pdata) {
			data = --(*pdata);
		}
		return data;
	}

public:
	/**
	 * check if data or event is set
	 **/
	bool		Event(int type) { return mWaitEvent.registed(type); }

	/**
	 * wakeup event and check if current event is done
	 **/
	bool		EventWakeup(int type) {
		/** wakeup a event success at least */
		if (Wakeup(type) >= 0) {
			return mWaitEvent.event_done(type);
		}
		return false;
	}

	/**
	 * set wait condition
	 **/
	void		Regist(int type, typeid_t unique = 0, int index = 0) {
		mWaitEvent.regist(type, unique);
	}

	/**
	 * set wait condition
	 **/
	void		Regist(std::initializer_list<int> array, typeid_t unique = 0, int index = 0) {
		mWaitEvent.regist(array, unique);
	}

	/**
	 * regist event handle
	 **/
	void		RegistHandle(int type, const common::WaitEvent::handle_t& handle) {
		/** regist wakeup handle */
		if (type == common::WaitEvent::WE_wake) {
			mWaitEvent.regist_handle(handle);
		} else {
			mWaitEvent.regist_handle(type, handle);
		}
	}

	/**
	 * set wait condition
	 **/
	void		RegistUnexpect(int type, int index = 0) {
		mWaitEvent.regist_unexpect(type);
	}

	/**
	 * set wait condition
	 **/
	void		RegistUnexpect(std::initializer_list<int> array, int index = 0) {
		mWaitEvent.regist_unexpect(array);
	}

	/**
	 * set event wait count
	 **/
	void		Count(int type, typeid_t unique = 0) {
		mWaitEvent.regist_count(type, unique);
	}

	/**
	 * wait and sleep
	 **/
	int			Wait(int time = 0, int index = 0) {
		int ret = mWaitEvent.wait(time == 0 ? mWaitTime : time);
		if (ret != 0 && mDumpWait) {
			DumpWaitEvent();
		}
		return ret;
	}

	/**
	 * wakeup event, maybe into wait
	 **/
	int			Wakeup(int type, typeid_t unique = 0, int index = 0) {
		return mWaitEvent.wakeup(type, unique);
	}

	/**
	 * dump wait event
	 **/
	void		DumpWaitEvent();

public:
	/**
	 * regist sync event
	 *
	 * @note when event wakeup, will notify and trigger this, now regist a new event
	 * 	and event handle, test thread will not wait for total event cond, but additional one in base mock
	 **/
	void		RegistSync(int type, typeid_t unique = 0) {
		mWaitEvent.regist_handle(std::bind(&BaseMock::SyncRegist, this, type));
	}

	/**
	 * when trigger wakeup, test thread wakeup until worker wait at sync point
	 **/
	int		SyncWait(int time = 0) {
		int ret = Wait(time);

		/** only when wakeup success, will enter to sync point */
		if (ret == 0) {

			debug("test thread wakeup, wait for sync");
			/** if wakeup failed, notify wakeup rightly */
            common::Mutex::Locker lock(Mutex());
			if (mSyncCond[0].wait_interval(Mutex(),
				time == 0 ? mWaitTime : time) != 0)
			{
				debug("test thread wait sync failed, sync done rightly");
				SyncDone();
			}
			debug("test thread wakeup by sync point, do sync work now");

		} else {
			debug("test thread wakeup failed, no need wait sync point");
			SyncDone();
		}

		Reset();
		return ret;
	}

	/**
	 * test thread notify worker, to work again
	 **/
	void		SyncDone() {
		debug("test thread done in sync point, wakeup worker again");
        common::Mutex::Locker lock(Mutex());
		mSyncCond[1].signal_all();
	}

protected:
	/**
	 * delay regist new event and handle
	 **/
	void		SyncRegist(int type) {
		debug("event wakeup, worker `regist sync point, type " << type);
		mWaitEvent.regist_handle(type, std::bind(&BaseMock::SyncEvent, this, type, 0));
	}

	/**
	 * when trigger wakeup, enter into sync
	 **/
	void		SyncEvent(int type, typeid_t unique = 0) {
		debug("worker thread sync ===========>");
        common::Mutex::Locker lock(Mutex());
		mSyncCond[0].signal_all();
		mSyncCond[1].wait(Mutex());
		debug("worker thread sync <===========");
	}

	/**
	 * get inner mutex
	 **/
    common::Mutex& Mutex() { return mWaitEvent.mutex(); }

protected:
	/**
	 * clear all event
	 **/
	void		ClearEvent() {
		mWaitEvent.reset();
	}

public:
	uint64_t	mAllocer = {1};
	uint32_t	mWaitTime = {mock::c_mock_wait};
	/** dump event info if timeout */
	bool		mDumpWait = {true};
    /** step run next instance */
    bool        mStep = {false};

    common::WaitEvent   mWaitEvent;
    common::TypeMap<int, int>  mDataMap;
    common::TypeMap<int, std::string>  mStrMap;

	common::Cond 	mSyncCond[2];
	int		mSyncType = {0};
};

#define LOG_CODE_HPP_END
#include "Perform/LogHelper.hpp"

#define g_mock	GetMock()

/**
 * get mock instance
 **/
SINGLETON(BaseMock, GetMock)

/**
 * check if data > 0
 **/
inline bool	MockData(int type) {
	return Testing() && g_mock.Data(type) > 0;
}

/**
 * dec data and return true when data > 0
 **/
inline bool MockDecData(int type) {
	return Testing() && g_mock.DecData(type) >= 0;
}

/**
 * dec data and return true when data == 0
 **/
inline int MockDecTest(int type) {
	return Testing() && g_mock.DecData(type);
}

/**
 * check if data not set and event not regist
 **/
inline bool	NoMockData(int type) {
	return Testing() && !g_mock.Data(type);
}

/**
 * check if event is regist
 **/
inline bool	MockEvent(int type) {
	return Testing() && g_mock.Event(type);
}

/**
 * check if event is regist
 **/
inline bool	NoMockEvent(int type) {
	return Testing() && !g_mock.Event(type);
}

/**
 * wakeup event, and check if event wakeup, so each event trigger only once
 *
 * @note mostly used in judgement and tirgger some mock,
 * @note it will wakeup single event only once
 **/
inline bool	MockWakeupEvent(int type) {
	return Testing() && g_mock.EventWakeup(type);
}

/**
 * mock wakeup and check the unique
 **/
inline bool MockWakeupUnique(int type, typeid_t unique) {
	return Testing() && g_mock.Wakeup(type, unique) == 0;
}

/**
 * notify event happen, if all event is triggered, return true
 *
 * @return mock.wakeup > 0 if event is trigger; wakeup = 0 if all event done
 * @note count set event notify count
 **/
inline bool MockWakeup(int type, int count = 0) {
	if (Testing()) {
		do {
			int ret = g_mock.Wakeup(type);
			if (ret == common::WaitEvent::WE_wake) {
				return true;

			} else if (ret == common::WaitEvent::WE_none ||
				ret == common::WaitEvent::WE_done)
			{
				break;
			}
		} while (--count > 0);
	}
	return false;
}

#define MockWakeupEventH(type, handle) 	if (MockWakeupEvent(type)) { handle; }
#define MockWakeupH(type, handle) 		if (MockWakeup(type)) { handle; }


