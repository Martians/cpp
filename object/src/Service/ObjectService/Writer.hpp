
#pragma once

#include <string>

#include "ObjectService/ObjectUnit.hpp"
#include "Common/Util.hpp"
#include "Common/ThreadBase.hpp"
#include "Common/ThreadPool.hpp"
#include "CodeHelper/Bitset.hpp"

using std::string;

struct ObjectConfig;
class Reader;
class Writer;
class WriteTask;

/**
 * writer thread
 **/
class WriteThread : public ThreadPool::Thread
{
public:
	WriteThread(ThreadPool* pool, int index)
		: ThreadPool::Thread(pool, index) {}

	template<typename...T>
	void	set(std::tuple<T...> param) {
		std::tie(mWriter) = param;
		mUnit.Set(mWriter);
	}
public:
	/**
	 * retry task
	 **/
	int		RetryTask(WriteTask* task);

	/**
	 * do object write work
	 **/
	int	 	WriteObject(WriteTask* task);

	/**
	 * recover unit
	 **/
	int		RecoverUnit(UnitIndex& index);

	/**
	 * set inner error
	 **/
	int		Errno(int error);

	/**
	 * retry task
	 **/
	int		Retry(WriteTask* task);

	/**
	 * clear status when pause
	 **/
	virtual void pause() {
		mUnit.Clear();
	}

protected:
	/** writer pointer */
	Writer*		mWriter = {NULL};
	/** working unit */
	ObjectUnit	mUnit;
	/** current retry */
	int			mRetry = {0};
};


class WriteTask : public common::ThreadPool::Task
{
public:
	WriteTask() {}

	virtual ~WriteTask() { Clear(); }

public:
	/**
	 * malloc new task
	 **/
	static WriteTask* Malloc();

	/**
	 * cycle task
	 **/
	virtual void cycle();

	/**
	 * set object
	 **/
	void	Set(Object* object) {
		mObject = object;
		assert(object->mLength == (int32_t)object->mData.length());
	}

	/**
	 * retry current task
	 **/
	int		Retry(WriteThread* curr);

	/**
	 * work for task
	 **/
	virtual void work(common::ThreadPool::Thread* thread);

	/**
	 * task work done
	 **/
	virtual void done(int error = 0);

protected:
	/**
	 * clear resource
	 **/
	void	Clear() {
		if (mObject) {
			mObject->Dec();
			mObject = NULL;
		}
	}

public:
	bool	mRetry = {false};
	/** current object */
	Object* mObject = {NULL};
	#if OBJECT_PERFORM
		StadgeTimer mTimer;
	#endif
};

class RecoverTask : public common::ThreadPool::Task
{
public:
	/**
	 * malloc new task
	 **/
	static RecoverTask* Malloc();

	/**
	 * cycle task
	 **/
	virtual void cycle();

	/**
	 * set object
	 **/
	void	Set(const UnitIndex& index) { mIndex = index; }

	/**
	 * work for task
	 **/
	virtual void work(common::ThreadPool::Thread* thread);

protected:
	/**
	 * task work done
	 **/
	virtual void done(int error = 0) {}

public:
	UnitIndex	mIndex;
};

/**
 * write data to files
 **/
class Writer
{
public:
	Writer(ObjectConfig* config)
		: mConfig(config) {}

	virtual ~Writer() {
		Stop();
	}
public:
	enum WriterTag {
		WT_null = 0,
		WT_trace_stop,
	};
	/**
	 * start writer thread
	 **/
	void	Start();

	/**
	 * stop writer thread
	 **/
	void	Stop(bool wait = false);

	/**
	 * pause writer thread
	 **/
	int 	Pause(bool wait = false) {
		return mPool.pause(wait);
	}

	/**
	 * put new object to writer
	 **/
	int		Put(Object* object);

	/**
	 * get object for read
	 **/
	void	Get(Object* object) {}

public:
	/**
	 * get config
	 **/
	ObjectConfig& GlobalConfig() { return *mConfig; }

	/**
	 * get writer config
	 **/
	ObjectConfig::Writer& Config() { return mConfig->writer; }

	/**
	 * recover unit
	 **/
	virtual void RecoverUnit(const UnitIndex& index);

	/**
	 * writer thread recover work done
	 **/
	int		RecoverDone(const UnitIndex& index, bool success);

	/**
	 * alloc unit from reader
	 **/
	int		CommitUnit(const UnitIndex& index);

	/**
	 * alloc unit from reader
	 **/
	int		FetchUnit(UnitIndex& index, int wait);

	/**
	 * commit object work
	 **/
	int		CommitObject(Object* object);

	/**
	 * cancel all request
	 **/
	void	Cancel(int error);

	/**
	 * dump inner status
	 **/
	string&	DumpStatus(string& str);

	BITSET_DEFINE;
protected:
	/** object config */
	ObjectConfig* mConfig = {NULL};
	/** work thread pool */
	ThreadPool	mPool = {"wrt"};
	ThreadBase mThread;
};

