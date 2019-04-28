
#define LOG_CODE 0

#include <algorithm>
#include <iomanip>

#include "Common/Display.hpp"
#include "UnitTest/Mock.hpp"

#include "ObjectService/LogReader.hpp"
#include "ObjectService/Statistic.hpp"
#include "ObjectService/Control.hpp"
#include "ObjectService/Writer.hpp"
#include "ObjectService/Reader.hpp"
#include "ObjectService/Object.hpp"

using namespace common;

Reader::Reader(ObjectConfig* config)
	: mConfig(config)
{
	set_name("reader", 1);
	ResetTimer();
}

void
Reader::ResetTimer()
{
	ObjectConfig::Reader::Interval& time = Config().interval;
	set_wait(std::min({time.ping, time.recover,
		time.unit, time.object}));

	mWait[TM_ping].set(time.ping);
	mWait[TM_drop].set(Table().timeout / 2);

	mWait[TM_recovr].set(time.recover);
	mWait[TM_object].set(time.object);
	mWait[TM_unit].set(time.unit);
}

void
ReaderDump()
{
	static string str;
	GetReader()->DumpStatus(str);
	reader_info(str);
}

void
Reader::Stop(int wait)
{
	if (wait > 0) {
		Mutex::Locker lock(mutex());
		log_trace("reader stop, remain commit unit " << UnitCount(US_commit) + UnitCount(US_update)
				<< ", object " << WaitCommit());
		TimeRecord record;

		TimeCounter time(wait, 50);
		size_t term = 5000, done = WaitCommitTotal();
		size_t curr = 0, last = 0, none = 0;

		WakeupWork();
		while ((time.remain() && none < term / time.step()) &&
			(curr = WaitCommitTotal()) > 0)
		{
			mEmptyCond.wait_interval(mutex(), time.wait());
			if (last == curr) {
				none++;
				trace("reader stop, time left " << string_timer(time.rest(), false, false)
					<< ", no change cycle " << none << "/" << term / time.step());

			} else {
				none = 0;
				last = curr;
				trace("reader stop, time left " << string_timer(time.rest(), false, false) << ", wait remain " << curr);
			}
		}
		if (curr == 0) {
			log_info("reader stop, wait " << string_record(record) << " done " << done - curr);

		} else {
			log_info("reader stop, wait " << string_record(record) << " timeout, done " << done - curr);
		}
	}

	if (stop() == 0) {
		mThread.stop();
		ReaderDump();

		mDB.Close();
		Dropped();
	}
}

void
Reader::Dropped()
{
	{
		Mutex::Locker lock(mutex());
		mStatus = ST_drop;
	}
	GetWriter()->Pause(true);

	MockWakeup(WT_drop);
	reader_inc(RS_drop);

	Mutex::Locker lock(mutex());
	mEmptyCond.signal_all();

	for (auto& unit : mUnit) {
		unit.clear();
	}

	for (auto object : mTemp) {
		object->Dec();
	}
	mTemp.clear();

	for (auto object : mList) {
		object->Dec();
	}
	mList.clear();

	for (auto object : mRetryList) {
		object->Dec();
	}
	mRetryList.clear();

	mRetryCommand.clear();
	mWaitRecovr.Reset();
}

void
Reader::Restart()
{
	if (Status(ST_drop)) {
		mStatus = ST_work;
		MockWakeup(WT_restart);
		log_info("restart reader, try to prepare");

		GetWriter()->Start();
		Prepare();
	}
}

void
Reader::Start()
{
	mThread.start(GlobalConfig().global.dump, ReaderDump);
	start();
}

void*
Reader::entry()
{
	Prepare();

	while (waiting()) {
		if (PingServer() != 0) {
			continue;
		}

		UnitWork();

		ObjectWork();

		MockWakeup(WT_loop);
		//trace("loop");
	}
	return NULL;
}

int
Reader::PingServer()
{
	if (!Timeout(TM_ping) &&
		!Status(ST_drop))
	{
		return 0;
	}

	if (ExecutePing() == 0) {
		reader_loop(RS_ping_fail);
		MockWakeup(WT_ping);
		if (mPingFail > 0) {
			log_debug("ping server success, total failed " << mPingFail);
			mPingFail = 0;

		} else {
			trace("ping server success");
		}

		UpdateTime(TM_drop);
		Restart();

	} else {
		reader_inc(RS_ping_fail);
		mPingFail++;

		if (!Status(ST_drop)) {
			MockWakeup(WT_ping_failed);

			/** TODO: if the reason is not connect dropped, will assert here */
			if (Timeout(TM_drop)) {
				log_warn("ping server, but timeout, clear request");
				Dropped();

			} else {
				log_info("ping server failed");
			}
		}
	}
    return mDB.Errno();
}

int
Reader::ExecutePing()
{
#if 0
	log_trace("==========================================");
	if (Status(ST_work)) {
		TRIGGER_ONCE(
			set_wait(10);
			mWait[TM_ping].set(500);
			mWait[TM_drop].set(1000);
		);
		TRIGGER_RANGE(1, 5, -1);
	}
#endif
	/** TODO: server should report and update its information */
	mDB.MuteError(DE_connect_closed);
	int ret = mDB.Execute("update %s set %s = now() where %s = %d",
		Table().server_table.c_str(),
		Table().time.c_str(),
		Table().sid.c_str(),
		GlobalConfig().global.sid);
	mDB.MuteError(0);
	return ret;
}

void
Reader::WakeupWork()
{
	wakeup();

	for (auto& time : mWait) {
		time.wakeup();
	}
}

int
Reader::Prepare()
{
	Mutex::Locker lock(mutex());

	mStatus = ST_prepare;
	/** wait until prepare done */
	while (running()) {
		if (PrepareTable() == 0) {
			break;
		}
		wait_unlock();
	}

	if (running()) {
		WakeupTime(TM_recovr);

		RecoverUnit();
		mStatus = ST_recover;
	}

	WakeupWork();
	return 0;
}

int
Reader::PrepareTable()
{
	if (Connect() != 0) {
		return -1;
	}

	/** create sequence */
	mDB.IgnoreError(DE_already_exist);
	if (mDB.Execute("CREATE SEQUENCE %s "
		"MINVALUE 1 START 1 CACHE 1 NO CYCLE",
		Table().allocation.c_str()) != 0)
	{
		mDB.IgnoreError(0);
		log_warn("prepare table, create sequence " << Table().allocation
			<< " failed, error: " << mDB.Error());
		return -1;
	}
	mDB.IgnoreError(0);
	timer_inc(TS_create_table, mDB.LastTime());

	if (mDB.Execute("CREATE TABLE IF NOT EXISTS %s "
		"(%s int PRIMARY KEY, %s timestamp DEFAULT now())",
		Table().server_table.c_str(),
		Table().sid.c_str(),
		Table().time.c_str()) != 0)
	{
		log_warn("prepare table, create table " << Table().server_table
			<< " failed, error: " << mDB.Error());
		return -1;
	}
	timer_inc(TS_create_table, mDB.LastTime());

	if (mDB.Execute("CREATE TABLE IF NOT EXISTS %s "
		"(%s bigint DEFAULT nextval('%s') PRIMARY KEY, "
		"%s bigint DEFAULT 0, %s int REFERENCES %s(%s), "
		"%s int, %s int DEFAULT %d)",
		Table().unit_table.c_str(),
		Table().index.c_str(), Table().allocation.c_str(),
		Table().curr.c_str(), Table().sid.c_str(), Table().server_table.c_str(), Table().sid.c_str(),
		Table().old_sid.c_str(), Table().status.c_str(), UnitIndex::US_alloc) != 0)
	{
		log_warn("prepare table, create table " << Table().unit_table
			<< " failed, error: " << mDB.Error());
		return -1;
	}
	timer_inc(TS_create_table, mDB.LastTime());

	if (CreateObjectTable() != 0 || CreateObjectTable(true) != 0) {
		return -1;
	}

	if (RegistServer(GlobalConfig().global.sid) != 0) {
		return -1;
	}

	MockWakeup(WT_prepare_table);
	log_trace("prepare table");
    return 0;
}

int
Reader::CreateObjectTable(bool new_one)
{
	const char* name = !new_one ? Table().object_table.c_str() : Table().object_new.c_str();
	if (mDB.Execute("CREATE TABLE IF NOT EXISTS %s "
		"(%s varchar(%d) PRIMARY KEY, "
		"%s bigint REFERENCES %s(%s), "
		"%s int)",
		name, Table().name.c_str(), c_object_name_size,
		Table().index.c_str(), Table().unit_table.c_str(), Table().index.c_str(),
		Table().offset.c_str()) != 0)
	{
		log_warn("prepare table, create table " << name
			<< " failed, error: " << mDB.Error());
		return -1;
	}
	timer_inc(TS_create_table, mDB.LastTime());
    return 0;
}

int
Reader::RegistServer(int sid)
{
	/** TODO: regist local uuid and fetch sid from a sequence, uuid is the identifier
	 **/
	if (mDB.Execute("update %s set %s = now() where %s = %d; "
		"insert into %s(%s) select %d "
		"where not exists (select 1 from %s where %s = %d)",
		Table().server_table.c_str(), Table().time.c_str(), Table().sid.c_str(), sid,
		Table().server_table.c_str(), Table().sid.c_str(), sid,
		Table().server_table.c_str(), Table().sid.c_str(), sid) != 0)
	{
		log_warn("regist server, but set server id " << sid
			<< " failed, error: " << mDB.Error());
		return -1;
	}
	timer_inc(TS_create_table, mDB.LastTime());
	return 0;
}

int
Reader::TestClearTable()
{
	if (Connect() == 0) {
		mDB.IgnoreError(DE_not_exist);
		if (mDB.Execute("DROP SEQUENCE %s CASCADE",
			Table().allocation.c_str()) == 0)
		{
			mDB.DropTable(Table().server_table, true);
			mDB.DropTable(Table().unit_table, true);
			mDB.DropTable(Table().object_table, true);
			mDB.DropTable(Table().object_new, true);
		}
		mDB.IgnoreError(0);
		mDB.Close();
	}
	return mDB.Errno();
}

void
Reader::UnitWork()
{
	RecoverUnit();

	FetchUnit();

	CommitUnit();
}

void
Reader::WaitReady()
{
	Mutex::Locker lock(mutex());
	while (!Status(ST_work) && running()) {
		mEmptyCond.wait(mutex());
	}
}

void
Reader::SetWaitWork(int count)
{
	/** set first recover wait count */
	if (!mWaitRecovr.work) {
		mWaitRecovr.work = true;
		mWaitRecovr.time.reset();

		mWaitRecovr.wait = count;
		mWaitRecovr.curr = writer_count(WS_recovr_done);
		mWaitRecovr.commit = timer_stat(TS_commit_unit).count();
		mWaitRecovr.object = timer_stat(TS_commit_object).count();

		log_trace("recover unit, set wait count " << mWaitRecovr.wait
			<< ", record current committed unit " << mWaitRecovr.commit << ", object " << mWaitRecovr.object);
	}
}

bool
Reader::TryWork()
{
	if (UnitCount(US_free) >= (size_t)Config().free.max) {
		trace("try work, remain commit " << WaitCommit());

		/** all recover object committed */
		if (WaitCommit() == 0) {
			/** all recover unit done, maybe not commit yet */
			return writer_count(WS_recovr_done) >= mWaitRecovr.curr + mWaitRecovr.wait;
		}
	}
	return false;
}

void
Reader::ReadyWork(bool wakeup)
{
	if (!Status(ST_work)) {
		if (TryWork()) {
			mStatus = ST_work;
			wakeup = true;

			WakeupWork();
			ReaderDump();

			log_info("ready work, current free unit " << UnitCount(US_free)
					<< ", recover unit " << writer_count(WS_recovr_done) - mWaitRecovr.curr
					<< ", commit unit " << timer_stat(TS_commit_unit).count() - mWaitRecovr.commit
					<< " object " << string_count(timer_stat(TS_commit_object).count() - mWaitRecovr.object)
					<< ", using " << string_record(mWaitRecovr.time));
		} else {
			wakeup = false;
		}
	}

	if (wakeup) {
		MockWakeup(WT_notify_unit_emtpy);
		mEmptyCond.signal_all();
	}
}

bool
Reader::RecoverMoreUnit()
{
	if (Status(ST_drop)) {
		return false;

	} else if (!Timeout(TM_recovr)) {
		return false;

	} else if (RecoverCount() <= 0) {
		return false;
	}
	return true;
}

int64_t
Reader::RecoverCount()
{
	size_t limit = 0;
	if (Status(ST_recover)) {
		limit = (size_t)Config().free.max - UnitCount(US_recovr) - UnitCount(US_free);

	} else {
		if (UnitCount(US_recovr) < (size_t)Config().recover.concurrent) {
			limit = (size_t)Config().recover.step;
		}
	}
	return (int64_t)limit;
}

int
Reader::RecoverUnit()
{
	Mutex::Locker lock(mutex());
	if (!RecoverMoreUnit()) {
		return 0;
	}
	lock.unlock();

	/** TODO: if server restart, first time and fetch all unit alloc as its own sid */
	//(select %s,%s from %s where %s = %d or %s in "
	if (FetchRecover() == 0) {
		lock.lock();

        UnitIndex index;
		int count = PQntuples(mDB.Result());
		SetWaitWork(count);

		for (int i = 0; i < count; i++) {
			index.index = s2n<int64_t>(PQgetvalue(mDB.Result(), i, 0));
			index.curr = s2n<int64_t>(PQgetvalue(mDB.Result(), i, 1));
			int sid = s2n<int>(PQgetvalue(mDB.Result(), i, 2));

			success(mUnit[US_recovr].add(index));
			WriterRecover(index, sid);
		}

		if (count > 0) {
			timer_inc(TS_fetch_recover, count, mDB.LastTime());
			MockWakeup(WT_fetch_recover, count);
			log_info("fetch recover unit " << count << ", total: " << DumpUnit(US_recovr));
		} else {
			trace("fetch recover unit, but get nothing");
		}

	} else {
		log_warn("fetch recover, unit but failed, error: " << mDB.Error());
	}
	return mDB.Errno();
}

int
Reader::FetchRecover()
{
	/** fetch all unit of our last alive */
	if (Status(ST_prepare)) {
		trace("fetch recover, get all local unit of last start");

		return mDB.Execute("update %s main set %s = %d, %s = main.%s from "
				"(select %s,%s from %s where %s = %d and %s < %d) sub "
			"where main.%s = sub.%s returning main.%s, main.%s, main.%s",
			Table().unit_table.c_str(), Table().sid.c_str(), mConfig->global.sid,
			Table().old_sid.c_str(), Table().sid.c_str(),
			Table().index.c_str(), Table().sid.c_str(),
			Table().unit_table.c_str(),Table().sid.c_str(), mConfig->global.sid,
			Table().status.c_str(), UnitIndex::US_cmplt,
			Table().index.c_str(), Table().index.c_str(),
			Table().index.c_str(), Table().curr.c_str(), Table().old_sid.c_str());

	} else {
		assert(RecoverCount() > 0);

		return mDB.Execute("update %s main set %s = %d, %s = main.%s from "
				"(select %s,%s from %s where %s in "
					"(select %s from %s where age(now(), %s) > interval'%dms')"
				"and %s < %d limit %d) sub "
			"where main.%s = sub.%s returning main.%s, main.%s, main.%s",
			Table().unit_table.c_str(), Table().sid.c_str(), mConfig->global.sid,
			Table().old_sid.c_str(), Table().sid.c_str(),
			Table().index.c_str(), Table().sid.c_str(),
			Table().unit_table.c_str(), Table().sid.c_str(),
			Table().sid.c_str(), Table().server_table.c_str(),
			Table().time.c_str(), Table().timeout,
			Table().status.c_str(), UnitIndex::US_cmplt, (int)RecoverCount(),
			Table().index.c_str(), Table().index.c_str(),
			Table().index.c_str(), Table().curr.c_str(), Table().old_sid.c_str());
	}
}

int
Reader::WriterRecover(const UnitIndex& index, int32_t sid)
{
	MockWakeup(index.index);
	MockWakeup(sid);
	log_trace("fetch " << StringIndex(index) << " server " << sid << ", need recover");

	GetWriter()->RecoverUnit(index);
    return 0;
}

int
Reader::RecoverDone(const UnitIndex& index, bool success)
{
	Mutex::Locker lock(mutex());
	success(mUnit[US_recovr].del(index));

	bool wakeup = (UnitCount(US_free) == 0);
	if (index.Status(UnitIndex::US_cmplt)) {
		log_info("recover done, commit full " << StringIndex(index));
		return CommitUnit(index, true);

	/** unit recoverd more data, try to commit */
	} else if (index.Status(UnitIndex::US_recov)) {
		mUnit[US_update].del(index);
		mUnit[US_update].add(index);

		log_info("recover done, " << StringIndex(index)
			<< ", update recover point to " << string_size(index.curr)
			<< ", free size " << index.size - index.curr
			<< ", remain " << UnitCount(US_recovr));
	} else {
		log_trace("recover done, " << StringIndex(index)
			<< ", keep recover point " << string_size(index.curr)
			<< ", remain " << UnitCount(US_recovr));
	}
	success(mUnit[US_free].add(index));
	reader_inc(RS_free);

	/** TODO： need wakeup fetch unit */
	MockWakeup(WT_recover_done);
	ReadyWork(wakeup);
	return 0;
}

int
Reader::FetchUnit(UnitIndex& index, int wait)
{
	Mutex::Locker lock(mutex());
	if (Disabled()) {
		log_trace("writer fetch unit, but reader is stopping, ignore");
		return -1;
	}
	TimeRecord 	record;
	TimeCounter time(wait, std::min(Config().interval.fetch, wait));

#if 0
	TRIGGER_RANGE(1, 10, -1);
	TRIGGER_RANGE(6, 15, -1);
#endif

	bool output = false;
	while (UnitCount(US_free) == 0) {
		if (running() && time.remain()) {
			if (!output) {
				output = true;
				log_debug("writer fetch unit, wait for free unit");
			}

			wakeup_unlock();
			mEmptyCond.wait_interval(mutex(), time.wait());

			MockWakeup(WT_fetch_unit_wait);
		} else {
			break;
		}
	}

	if (running() && UnitCount(US_free) > 0) {
		index = *mUnit[US_free].minv();

		success(mUnit[US_free].del(index));
		success(mUnit[US_using].add(index));
		reader_inc(RS_used);

		if (output) {
			log_debug("writer fetch unit, get free " << StringIndex(index) << ", wait " << string_record(record));
		} else {
			log_debug("writer fetch unit, get free " << StringIndex(index));
		}
		return 0;

	} else {
		log_info("writer fetch unit, timeout " << string_timer(wait, false, false));
		return -1;
	}
}

bool
Reader::FetchMoreUnit()
{
	if (Status(ST_drop)) {
		return false;

	} else if (Status(ST_work)) {
		if (UnitCount(US_free) >= (size_t) Config().free.min) {
			return false;
		}
	}

	if (UnitCount(US_free) + UnitCount(US_recovr) >= (size_t)Config().free.max) {
		return false;

	} else if (MockData(WT_fetch_unit_tout)) {
		return false;
	}
	return true;
}

int
Reader::FetchUnit()
{
	Mutex::Locker lock(mutex());
    if (!FetchMoreUnit()) {
    	return 0;
    }
    int count = Config().free.max - (int)(UnitCount(US_free) + UnitCount(US_recovr));

    /** TODO: maybe reuse unit */
    mDB.Prepare("insert into %s(%s) values",
		Table().unit_table.c_str(),
		Table().sid.c_str());
    const char* sep = "";
	while (count-- > 0) {
		mDB.Append("%s(%d)", sep, mConfig->global.sid);
		sep = ",";
	}
    mDB.Append(" RETURNING %s", Table().index.c_str());
    lock.unlock();

	if (mDB.Execute() == 0) {
		lock.lock();

        UnitIndex unit;
		bool wakeup = (UnitCount(US_free) == 0);
		int count = PQntuples(mDB.Result());
		reader_inc(RS_free, count);
		for (int i = 0; i < count; i++) {
			unit = s2n<int64_t>(PQgetvalue(mDB.Result(), i, 0));
			success(mUnit[US_free].add(&unit));
		}
		ReadyWork(wakeup);

		timer_inc(TS_fetch_free, count, mDB.LastTime());
		MockWakeup(WT_fetch_free, count);
		log_info("fetch free unit " << count << ", total : " << DumpUnit(US_free)
				<< " using " << string_timer(mDB.Time()));
	} else {
		log_warn("fetch free unit, but failed, error: " << mDB.Error());
	}
	return mDB.Errno();
}

int
Reader::CommitUnit(const UnitIndex& index, bool recover)
{
	Mutex::Locker lock(mutex(), !recover);
	if (Disabled()) {
		log_trace("commit unit, but reader is stopping, ignore");
		return -1;
	}

	if (recover) {
		success(mUnit[US_commit].add(index));

	} else {
		success(mUnit[US_commit].add(index));
		success(mUnit[US_using].del(index));
		log_debug("try to commit " << StringIndex(index));
	}

	if (TryCommitUnit(true)) {
		wakeup_unlock();
	}
	return 0;
}

bool
Reader::TryCommitUnit(bool check)
{
	if (Status(ST_drop)) {
		return false;
	}

	if (Timeout(TM_unit, check)) {
		if (UnitCount(US_commit) + UnitCount(US_update) == 0) {
			return false;
		}
		MockWakeup(WT_commit_unit_wait);

	} else {
		if (UnitCount(US_commit) + UnitCount(US_update) < (size_t)Config().commit.unit) {
			return false;
		}
		MockWakeup(WT_commit_unit_batch);
	}
	return true;
}

void
Reader::CommitUnit()
{
	Mutex::Locker lock(mutex());
	if (!TryCommitUnit()) {
		return;
	}

	mDB.Prepare("");
	UpdateUnit(US_commit);
	UpdateUnit(US_update);
	lock.unlock();

	if (mDB.Execute() == 0) {
		lock.lock();

		MockWakeup(WT_commit_unit, (int)UnitCount(US_commit));
		log_info("commit unit, count " << UnitCount(US_commit) + UnitCount(US_update)
			<< ", commit: " << DumpUnit(US_commit)
			<< ", update: " << DumpUnit(US_update)
			<< " using " << string_timer(mDB.Time()));

		timer_inc(TS_commit_unit, mUnit[US_commit].size() +
				mUnit[US_update].size(), mDB.LastTime());
		mUnit[US_commit].clear();
		mUnit[US_update].clear();

	} else {
		MockWakeup(WT_commit_unit_failed);
		log_warn("commit unit, count " << UnitCount(US_commit) + UnitCount(US_update)
			<< ", but failed, using " << string_timer(mDB.Time()));
	}
}

void
Reader::UpdateUnit(int type)
{
	/**
	 * if unit already be complete, could ignore
	 *
	 * @note if not exist, we should insert it, and record unexpected situation
	 * @note record committing server, time for later trace
	 **/
	int status = (type == US_commit) ?
		UnitIndex::US_cmplt : UnitIndex::US_using;

	for (auto& index: mUnit[type]) {
		mDB.Append("update %s set %s = %d, %s = %" i64 " where %s = %" i64 "; "
			"insert into %s(%s, %s, %s, %s) select %" i64 ", %d, %d, %" i64 " "
				"where not exists (select 1 from %s where %s = %" i64 "); ",
			Table().unit_table.c_str(), Table().status.c_str(), status,
			Table().curr.c_str(), index.curr, Table().index.c_str(), index.index, Table().unit_table.c_str(),
			Table().index.c_str(), Table().sid.c_str(), Table().status.c_str(), Table().curr.c_str(),
			index.index, GlobalConfig().global.sid, status, index.curr,
			Table().unit_table.c_str(), Table().index.c_str(), index.index);
	}
}

int
Reader::QueryObject()
{
	return 0;
}

int
Reader::CommitObject(Object* object)
{
	#if OBJECT_PERFORM
		return 0;
	#endif
	object->Inc();

	#if OBJECT_PERFORM
		common::time_reset();
	#endif

	Mutex::Locker lock(mutex());
	if (Disabled()) {
		object->Dec();
		trace("commit object, but reader is stopping, ignore");
		return -1;
	}
	mTemp.enque(object);

	if (TryCommitObject(true)) {
		wakeup_unlock();
	}

	#if OBJECT_PERFORM
		common::time_reset();
		if (common::time_last() > 500) {
			log_info("commit object ==========================" << common::time_last());
		}
	#endif
	return 0;
}

bool
Reader::TryCommitObject(bool check)
{
	if (Status(ST_drop)) {
		return false;
	}

	size_t wait_count = WaitCommit();
	if (Timeout(TM_object, check)) {
		if (wait_count == 0) {
			//trace("commit object, wait timeout, no request");
			return false;
		}
		trace("commit object, wait timeout" << (mWait[TM_object].wake() ? ", wakeup" : ""));
		MockWakeup(WT_commit_object_wait);

	} else {
		if (wait_count < (size_t)Config().commit.batch) {
			trace("try commit object, count " << wait_count << ", wait batch");
			return false;
		}
		trace("commit object, wait exceed batch, " << wait_count);
		MockWakeup(WT_commit_object_batch);
	}
	return true;
}

void
Reader::CommitObject()
{
	Mutex::Locker lock(mutex());
	if (!TryCommitObject()) {
		return;
	}
	mList.enque(mTemp, true);
	mTemp.clear();
	lock.unlock();

	/**
	 * TODO:
	 * 	if insert duplicate name, should check if index be the same, or rewrite happen,
	 * 		move old value to cycle table
	 * 	if some success, some failed, split them and record
	 * 	if index and offset already exist, should put the old value to other table
	 * */
	const char* sep = "";
	if (mRetryCommand.length() == 0) {
		mDB.Prepare("with %s (%s, %s, %s) as (values ",
			Table().object_new.c_str(), Table().name.c_str(), Table().index.c_str(), Table().offset.c_str());
	} else {
		trace("swap command: " << mRetryCommand);
		mDB.SwapCommand(mRetryCommand);
	}

	Object* object = NULL;
	while (mDB.Command().length() < (size_t)Config().commit.limit &&
		mRetryList.size() < (size_t)Config().commit.batch)
	{
		if (!(object = mList.deque())) {
			break;
		}
		mRetryList.enque(object);

		mDB.Append("%s('%s', %" i64 ", %d)", sep, object->mKey.name,
			object->mLocation.index.index, object->mLocation.offset);
		sep = ",";
	}
	mDB.Append(Upsert().c_str());
	//lock.unlock();

	if (mDB.Execute(false) == 0) {
		lock.lock();

		timer_inc(TS_commit_object, mRetryList.size(), mDB.LastTime());
		MockWakeup(WT_commit_object, mRetryList.size());
		log_debug("commit object, count " << mRetryList.size()
			<< " length " << string_size(mDB.Command().length())
			<< " remain " << mList.size() << "  - " << string_timer(mDB.Time()));
    
        Object* object = NULL;
		while ((object = mRetryList.deque())) {
			object->Dec();
		}
		mDB.Command().clear();
		ReadyWork(false);

		/** is still remain request, trigger another commit object rightly,
		 *  maybe request in list already exceed timeout, but not exceed batch limit */
		// TryCommit(true)
		if (mList.size() > 0) {
			MockWakeup(WT_commit_object_wakeup);
			trace("commit object, remain " << mList.size() << ", wakeup");

			if (mList.size() >= (size_t)Config().commit.batch) {
				wakeup();
				WakeupTime(TM_object);

			} else {
				int next_wait = mWait[TM_object].wait() / 3;
				wake_next(next_wait);
				mWait[TM_object].next(next_wait);
			}
		}

	} else {
		MockWakeup(WT_commit_object_failed);

		mDB.SwapCommand(mRetryCommand);
		mRetryCommand.resize(mRetryCommand.length() - Upsert().length());
		log_info("commit object, count " << mRetryList.size()
			<< " length " << string_size(mRetryCommand.length())
			<< ", but failed");
	}
}

const std::string&
Reader::Upsert()
{
	static const std::string upsert = format("), "
	"upsert as ("
	    "update %s as origin "
			"set %s = new.%s, %s = new.%s, %s = new.%s "
			"FROM %s new "
			"WHERE origin.%s = new.%s "
			"RETURNING origin.*"
	")"
	"INSERT INTO object (%s, %s, %s) "
		"SELECT %s, %s, %s FROM %s "
		"WHERE NOT EXISTS ("
			"SELECT 1 FROM upsert up "
			" WHERE up.%s = %s.%s)",
	//update %s set %s = new.%s
	Table().object_table.c_str(), Table().name.c_str(), Table().name.c_str(),
	//%s = new.%s, %s = new.%s: index = new.index, off = new.off
	Table().index.c_str(), Table().index.c_str(), Table().offset.c_str(), Table().offset.c_str(),
	// FROM %s new WHERE origin.%s = new.%s
	Table().object_new.c_str(), Table().name.c_str(), Table().name.c_str(),
	//INSERT INTO object (name, index, off)
	Table().name.c_str(), Table().index.c_str(), Table().offset.c_str(),
	//SELECT %s, %s, %s FROM new_value
	Table().name.c_str(), Table().index.c_str(), Table().offset.c_str(), Table().object_new.c_str(),
	//"WHERE NOT EXISTS (""SELECT 1 FROM upsert up"" WHERE up.%s = %s.%s)
	Table().name.c_str(), Table().object_new.c_str(), Table().name.c_str());
	return upsert;
}

void
Reader::ObjectWork()
{
	QueryObject();

	CommitObject();
}

int
Reader::Get(const string& object, Location& location)
{
	if (mDB.Execute("select %s, %s from %s where %s = %s",
		Table().index.c_str(),
		Table().offset.c_str(),
		Table().object_table.c_str(),
		Table().name.c_str(),
		object.c_str()) != 0)
	{
        return -1;
	}
	assert(PQntuples(mDB.Result()) == 1);
	location.index  = mDB.SingleInt64(0, 0);
	location.offset = (int)mDB.SingleInt64(0, 1);
	return 0;
}

void
Reader::Del(const string& object)
{
	// create table xdual(id int, update_time timestamp(0))
	// update xdual set update_time=now();
}

std::string
Reader::DumpUnit(int type)
{
	TypeSet<UnitIndex>& set = mUnit[type];
	std::stringstream ss;

	ss << "[";
	const char* sep = "";
	for (auto& index: set) {
		ss << sep << index.index;
		sep = ", ";
	}
	ss << "]";
	return ss.str();
}

string&
Reader::DumpStatus(string& str)
{
	Mutex::Locker lock(mutex());
    format(str, "reader status:"
    	"\n\t recovr:  %8" i64 ", %8" i64 "  %s"
    	"\n\t commit:  %8" i64 ", %8" i64 "  %s"
		"\n\t using:   %8" i64 ", %8s  %s"
		"\n\t free:    %8" i64 ", %8s  %s"
		"\n\t object:  %8" i64 ", %8" i64,
		writer_count(WS_recovr_recv) - writer_count(WS_recovr_done), writer_count(WS_recovr_done), DumpUnit(US_recovr).c_str(),
		UnitCount(US_commit), timer_stat(TS_commit_unit).count(), DumpUnit(US_commit).c_str(),
		UnitCount(US_using), string_size(reader_count(RS_used) * GlobalConfig().writer.unit).c_str(), DumpUnit(US_using).c_str(),
		UnitCount(US_free), string_size(reader_count(RS_free) * GlobalConfig().writer.unit).c_str(), DumpUnit(US_free).c_str(),
		WaitCommit(), timer_stat(TS_commit_object).count());

    return str;
}


