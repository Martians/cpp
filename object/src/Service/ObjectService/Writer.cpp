
#include "ObjectService/Writer.hpp"

#include "ObjectService/Config.hpp"
#include "ObjectService/Control.hpp"
#include "ObjectService/LogWriter.hpp"
#include "ObjectService/Reader.hpp"
#include "ObjectService/Statistic.hpp"
#include "Common/Display.hpp"
#include "Advance/TypeAlloter.hpp"
#include "UnitTest/Mock.hpp"


enum ObjectTaskType
{
	OT_task,
	OT_recover,
};

static BaseAlloter*
TaskPool(int type = OT_task)
{
	switch (type) {
	case OT_task: {
		static TypeAlloter<WriteTask> s_task(4096);
		return &s_task;
	} break;
	case OT_recover: {
		static TypeAlloter<RecoverTask> s_task(64);
		return &s_task;
	} break;
	default: assert(0);
		break;
	}
	return NULL;
}

WriteTask*
WriteTask::Malloc()
{
	return (WriteTask*)TaskPool()->_new();
}

void
WriteTask::cycle()
{
	TaskPool()->_del(this);
}

void
WriteTask::work(common::ThreadPool::Thread* thread)
{
	#if OBJECT_PERFORM
		mTimer.reset(true);
		mTimer.next("work last");
	#endif

	WriteThread* curr = (WriteThread*)thread;
	curr->WriteObject(this);
	
	#if OBJECT_PERFORM
		mTimer.next("write object", 10000);
	#endif
	/** no need retry, done work */
	if (mRetry) {
		Retry(curr);

	} else {
		done();
	}

	#if OBJECT_PERFORM
		mTimer.next("clear object");
		mTimer.bomb("writer check task");
	#endif
}

int
WriteTask::Retry(WriteThread* curr)
{
	inc();
	mRetry = false;
	mObject->mError = 0;

	return curr->pool()->add(this, true);
}

void
WriteTask::done(int error)
{
	/** if error == 0, keep object error */
	if (error == 0) {
		trace("set task done, keep origin error " << mObject->mError);

	} else {
		mObject->mError = error;
		trace("set task done, error " << error);
	}
	mObject->Done();
	Clear();
}

RecoverTask*
RecoverTask::Malloc()
{
	return (RecoverTask*)TaskPool(OT_recover)->_new();
}

void
RecoverTask::cycle()
{
	TaskPool(OT_recover)->_del(this);
}

void
RecoverTask::work(common::ThreadPool::Thread* thread)
{
	WriteThread* curr = (WriteThread*)thread;
	curr->RecoverUnit(mIndex);
	done();
}

int
WriteThread::Errno(int error)
{
	if (error != 0) {
		mWriter->Cancel(error);
	}
    return error;
}

int
WriteThread::RetryTask(WriteTask* task)
{
	if (mRetry >= GetConfig()->writer.task.retry) {
		log_warn("retry task, but already exceed retry limit " << mRetry);
		return -1;
	}
	mRetry++;

	bool retry = false;
	Object* object = task->mObject;

	switch (object->mError) {
	case OS_alloc_unit:
		retry = true;
		break;
	default:
		break;
	}

	if (retry) {
		writer_inc(WS_object_retry);

		task->mRetry = true;
		log_info("retry task, key " << object->mKey.name << ", retry " << mRetry);
		return 0;

	} else {
		writer_inc(WS_object_fail);
		return -1;
	}
}

int
WriteThread::WriteObject(WriteTask* task)
{
	int ret = mUnit.Write(task->mObject);
	if (ret != 0) {
		ret = RetryTask(task);

	} else {
		mRetry = 0;
		writer_inc(WS_object_done);
	}
	return Errno(ret);
}

int
WriteThread::RecoverUnit(UnitIndex& index)
{
	int ret = mUnit.Recover(index);
	return Errno(ret);
}

void
WriterDump()
{
	static string str;
	GetWriter()->DumpStatus(str);
	writer_info(str);
}

void
Writer::Start()
{
	//mConfig->writer.thread = 1;
	int ret = mPool.start<WriteThread>(mConfig->writer.thread, this);
	if (ret == 0) {
		mThread.start(GlobalConfig().global.dump, WriterDump);
	}
	reset_bit();
}

void
Writer::Stop(bool wait)
{
	if (mPool.stop(wait) == 0) {
		mThread.stop();
		WriterDump();
	}
}

void
Writer::Cancel(int error)
{
	if (Pause(false)) {
		log_warn("writer occur error " << error << ", pause and cancel all request");
	}
}

int
Writer::Put(Object* object)
{
	WriteTask* task = WriteTask::Malloc();
	task->Set(object);

	if (mPool.add(task) != 0) {
		if (change(WT_trace_stop, true)) {
			log_trace("put task, but pool is stopping");
		}
		return -1;
	}

	writer_inc(WS_object_recv);
	writer_inc(WS_object_size, object->mLength + c_object_head_size);
	return 0;
}

int
Writer::CommitUnit(const UnitIndex& index)
{
	return GetReader()->CommitUnit(index);
}

int
Writer::FetchUnit(UnitIndex& index, int wait)
{
	return GetReader()->FetchUnit(index, wait);
}

int
Writer::CommitObject(Object* object)
{
	return GetReader()->CommitObject(object);
}

void
Writer::RecoverUnit(const UnitIndex& index)
{
	writer_inc(WS_recovr_recv);
	RecoverTask* task = RecoverTask::Malloc();
	task->Set(index);
	mPool.add_prior(task);
}

int
Writer::RecoverDone(const UnitIndex& index, bool success)
{
	writer_inc(WS_recovr_done);
	return GetReader()->RecoverDone(index, success);
}

string&
Writer::DumpStatus(string& str)
{
    format(str, "writer status:"
    	"\n\t recover: %8" i64 ", \tcrash: %8" i64 ", \t trunc: %8" i64 ", \t object: %8s"
    	"\n\t                     \t read: %8s, \t span:  %8s, \t trunc:  %8" i64 ", \t fail:  %8" i64
    	"\n\t request: %8" i64 ", \t done: %8s, \t retry: %8" i64 ", \t fail:   %8" i64
		"\n\t write:   %8s, \t done: %8s",
		writer_count(WS_recovr_done), writer_count(WS_recovr_head_partial) + writer_count(WS_recovr_object_head_crash),
		writer_count(WS_recovr_object_trunc), string_count(writer_count(WS_recovr_object)).c_str(),
		string_size(writer_count(WS_recovr_read)).c_str(), string_size(writer_count(WS_recovr_span)).c_str(),
		writer_count(WS_recovr_trunc), writer_count(WS_recovr_read_failed),
		writer_count(WS_object_recv) - writer_count(WS_object_done), string_count(writer_count(WS_object_done)).c_str(),
        writer_count(WS_object_retry), writer_count(WS_object_fail),
		string_size(writer_count(WS_object_size) - writer_count(WS_object_size_done)).c_str(), string_size(writer_count(WS_object_size_done)).c_str());
	 return str;
}

