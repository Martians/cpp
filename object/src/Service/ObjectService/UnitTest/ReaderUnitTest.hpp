
#pragma once

#include "ObjectUnitTest.hpp"

#define READER_HOST	"127.0.0.1"
#define READER_USER	"postgres"
#define READER_PWD	"111111"
#define READER_DB	"mydb"
#define READER_WAIT	1

namespace object_mock {

	enum UnitStat {
		US_timeout = 0,
		US_working,
		US_prehead,
		US_complet,
		US_end,
	};
	const int c_unit_stat_time[] = { -100000, 0, 100000, -2000000 };

	const int c_thread_wait	= 1;
	const int c_ping_wait	= 3;
	const int c_ping_time_out = 20;
	const int c_unit_time_out = 50;

	const int c_commit_object_time	= 30;
	const int c_commit_object_batch	= 10;
	const int c_commit_unit_time	= 30;
	const int c_commit_unit_batch	= 5;
	const int c_commit_unit_start	= 2;
	const int c_fetch_unit_wait		= 30;
	const int c_fetch_unit_count	= 20;
}
using namespace object_mock;

/**
 * database test
 **/
class DataBaseTest : public ObjectTest
{
public:
	DataBaseTest() {
		mConfig.reader.conn = SetConnInfo(READER_HOST, READER_USER, READER_PWD, READER_DB, READER_WAIT);
		mConfig.global.sid = SERVER_1;
	}

	virtual void SetUp() {
    	g_mock.SetWaitTime(1);
    }

    virtual void TearDown() {
    	g_mock.SetWaitTime();
    }

public:
	/**
	 * wait current event
	 **/
	int		Wait(int type) {
		/** regist event happen */
		int ret = g_mock.Wait();
		if (ret == 0) {
			/** error match */
			if (!mDB.CurrError(type)) {
				ret = -1;
			}
		}
		return ret;
	}

	/**
	 * wait event array
	 **/
	int		Wait(std::initializer_list<int> array) {
		for (const auto& type : array) {
			if (Wait(type) == 0) {
				return 0;
			}
		}
		return -1;
	}

public:
	/**
	 * mockdb connect
	 **/
	int		DBConnect() {
		mDB.IgnoreError(-1);
		return mDB.Connect(&mConfig.reader.conn);
	}

	/**
	 * create new table
	 **/
	int		CreateTable(const string& table) {
		return mDB.Execute("CREATE TABLE %s (index int)", table.c_str());
	}
protected:
	PostgreSQL		mDB;
};

/**
* reader for test, just expose protected interface
**/
class MockReader : public Reader
{
public:
	MockReader(ObjectConfig* config) : Reader(config) {}

public:
	int		PrepareTable() { return Reader::PrepareTable(); }

	void	MockDrop() { mDB.MockDrop(); }

	int		CommittingUnit() {
		return (int)mUnit[US_commit].size();
	}

	int		CommittingObject() {
		return (int)mList.size();
	}

public:
	void	PingTest(bool short_time = true) {
		set_wait(c_thread_wait);

		mWait[TM_ping].set(c_ping_wait);

		if (short_time) {
			mWait[TM_drop].set(c_ping_time_out);
		} else {
			mWait[TM_drop].set(c_mock_wait - 100);
		}
		mConfig->table.timeout = c_unit_time_out;
	}

	void	CommitObjectTest(bool short_time = true) {
		set_wait(c_thread_wait);

		/** sync wait point */
		//mWait[TM_ping].set(c_ping_wait);

		if (short_time) {
			mWait[TM_object].set(c_commit_object_time);
		} else {
			mWait[TM_object].set(c_mock_wait);
		}
		mConfig->reader.commit.batch = c_commit_object_batch;
	}

	void	CommitUnitTest(bool short_time = true) {
		set_wait(c_thread_wait);

		/** sync wait point */
		//mWait[TM_ping].set(c_ping_wait);

		if (short_time) {
			mWait[TM_unit].set(c_commit_unit_time);
		} else {
			mWait[TM_unit].set(c_mock_wait);
		}
		mConfig->reader.commit.unit = c_commit_unit_batch;
	}

	virtual int	CommitUnit(const UnitIndex& index, bool locked = false) {
		assert(mUnit[US_using].add(index));
		return Reader::CommitUnit(index, false);
	}
};

class MockWriter : public Writer {
public:
	 MockWriter(ObjectConfig* config) : Writer(config) {

	 }

public:
	 virtual void RecoverUnit(const UnitIndex& index) {
		 RecoverDone(index, true);
	 }
};

/**
 * reader test
 **/
class ReaderServer : public DataBaseTest
{
public:
	ReaderServer() : mReader(&mConfig) {
		mWriter = new MockWriter(&mConfig);
		GetControl()->SetWriter(mWriter);
		GetControl()->SetReader(&mReader);

		ClearTable();
	}

	virtual void SetUp() {
    }

    virtual void TearDown() {
    	Stop();
    	GetControl()->SetReader(NULL);
    	GetControl()->SetWriter(NULL);
		reset(mWriter);
    }

public:
	void	Start() {
		log_info("----------------------- prepare -----------------------");
		mReader.Start();
	}

	void	Stop() { mReader.Stop(); }

	/**
	 * establish table
	 **/
	void	CreateTable() {
		mReader.PrepareTable();
		mReader.Stop();
	}

	/**
	 * drop all
	 **/
	void	ClearTable() {
		assert(mReader.TestClearTable() == 0);
	}

public:
	struct Server
	{
		int32_t	  sid;
		int		  status;
	};

	struct UnitAlloc
	{
		UnitIndex index;
		int32_t	  sid;
	};

	/** for keep unitindex and status */
	typedef std::tuple<UnitIndex, int> UnitStatus;

	/**
	 * establish test env
	 **/
	void	Establish() {
		mReader.PrepareTable();
		DBConnect();

		/** set server status */
		ServerStatus({
			{US_working, {SERVER_1}},
			{US_timeout, {SERVER_2, SERVER_4}},
			{US_prehead, {SERVER_3}},
			{US_complet, {SERVER_5}}
		});

		/** set unit index alloc */
		PrepareUnit({ {1, SERVER_2}, {2, SERVER_2}, {3,  SERVER_1}, {6, SERVER_4},
				  {7, SERVER_3}, {8, SERVER_3}, {10, SERVER_1} });
		PrepareUnit({ {4, SERVER_4}, {5, SERVER_1}, {9,  SERVER_2}}, UnitIndex::US_cmplt);

		mReader.Stop();
	}

	void	WaitStart() {
		g_mock.Regist({WT_fetch_recover});
		g_mock.RegistSync(WT_loop);

		Start();
		EXPECT_EQ(0, g_mock.SyncWait());
	}

	void	RegistWait(int type, int time) {
		g_mock.RegistHandle(type, std::bind(usleep, time * 1000));
	}

	/**
	 * wait until establish complete
	 **/
	void	WaitRecover() {
		Establish();

		g_mock.Regist({WT_fetch_recover});
		/** wait on a success ping */
		g_mock.RegistSync(WT_loop);

		Start();

		EXPECT_EQ(0, g_mock.SyncWait());
	}

protected:
	/**
	 * ping test
	 **/
	void	PingTest(bool short_time = true) {
		mReader.PingTest(short_time);
		WaitRecover();
	}

	/**
	 * drop current
	 **/
	void	ConnDrop(bool timeout = false) {
		g_mock.SetData(WT_drop_trigger, timeout ?
			c_ping_time_out/c_ping_wait + 1 : 1);
		g_mock.Regist(WT_ping_failed);

		mReader.MockDrop();
	}

	/**
	 * set server status
	 **/
	void	ServerStatus(std::initializer_list<MapContain<int, int>::Pair> array) {
		mServerMap.regist(array);
		for (const auto& pair : array) {
			for (const auto& server : pair.set) {
				success(mReader.RegistServer(server) == 0);
				success(mDB.Execute("update %s set %s = now() + interval '%dms' where %s = %d",
					mConfig.table.server_table.c_str(), mConfig.table.time.c_str(), c_unit_stat_time[pair.key],
					mConfig.table.sid.c_str(), server) == 0);
			}
		}
	}

	/**
	 * prepare unit
	 **/
	void  	PrepareUnit(std::initializer_list<UnitAlloc> array, int status = UnitIndex::US_alloc) {
		for (const auto& stat : array)  {
			UnitIndex index;
			index.index = stat.index.index + 100;
			AllocUnit(index.index, stat.sid, status);
			UnitStatus unit(index, status);
			success(mIndexMap.add(stat.sid, unit));
		}
	}

	/**
	 * alloc unit for server
	 **/
	void AllocUnit(int index, int server, int status) {
		success(mDB.Execute("insert into %s(%s, %s, %s) values(%d, %d, %d) returning %s",
			mConfig.table.unit_table.c_str(),
			mConfig.table.index.c_str(),
			mConfig.table.sid.c_str(),
			mConfig.table.status.c_str(),
			index, server, status,
			mConfig.table.index.c_str()) == 0);
	}

	/**
	 * regist server and index in the status
	 **/
	void	RegistEvent(int server_status, int unit_status) {
		/** loop container */
		int server;
		mServerMap.init(server_status);
		while (mServerMap.next(server)) {
			g_mock.Regist(server);
			RegistUnit(server, unit_status);
		}
	}

	void	RegistUnit(int server, int unit_status) {
		UnitStatus unit;
		mIndexMap.init(server);
		while (mIndexMap.next(unit)) {
			if (std::get<1>(unit) == unit_status) {
				g_mock.Regist(std::get<0>(unit).index);
			}
		}
	}

protected:
	MockReader	mReader;
	MockWriter*	mWriter = {NULL};
	MapContain<int, UnitStatus> mIndexMap = {-1};
	MapContain<int, int> mServerMap = {-1, -1};
};

class ReaderCommit : public ReaderServer
{
public:
	virtual void SetUp() {
		g_source.Init();
    }

	/**
	 * mock commit request
	 **/
	void	DoCommit(int count = 0, const byte_t* name = NULL) {
		do {
			Object* object = g_source.Next();
			if (name) {
				object->SetKey(name, (uint32_t)strlen(name));
			}
			mReader.CommitObject(object);
			object->mLocation.index = 2;
			object->mLocation.offset = random() % c_length_64M;
			object->Dec();
		} while (--count > 0);
	}

	/**
	 * mock commit unit
	 **/
	void	DoCommitUnit(int index, int count = 0) {
		do {
			mReader.CommitUnit(index);
			index++;
		} while (--count > 0);
	}

protected:
	/**
	 * prepare for commit
	 **/
	void	CommitObjectTest(bool short_time = true) {
		mReader.CommitObjectTest(short_time);

		WaitRecover();
	}

	/**
	 * prepare for commit
	 **/
	void	CommitUnitTest(bool short_time = true) {
		mReader.CommitUnitTest(short_time);

		WaitRecover();
	}

	/**
	 * prepare for duplicate test
	 **/
	void	DuplicateTest(int count) {
		/** reset commit object time */
		mReader.CommitObjectTest();
		g_mock.Regist(WT_commit_object);
		g_mock.RegistSync(WT_loop);

		/** wait origion commit success */
		Start();
		DoCommit(1, c_duplicate_value);
		ASSERT_EQ(0, g_mock.SyncWait());

		/** regist wait object commit success */
		g_mock.Regist(WT_commit_object);
		g_mock.Count(WT_commit_object, count);
		g_mock.RegistUnexpect(WT_commit_object_failed);
	}

	void	FetchUnitTest() {
		mConfig.reader.interval.fetch = 2;
		WaitStart();
	}

};

