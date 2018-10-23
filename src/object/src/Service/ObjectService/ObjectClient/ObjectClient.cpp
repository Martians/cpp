
#include "ObjectClient.hpp"

#include "Advance/TypeAlloter.hpp"
#include "Perform/StatThread.hpp"
#include "ObjectService/Control.hpp"
#include "ObjectService/Writer.hpp"

const utime_t BaseClient::s_wait = utime_t(30000);

static TypeAlloter<Context> s_context(4096);

Context*
Context::Malloc()
{
	return s_context.get();
}

void
Context::Cycle()
{
	return s_context.put(this);
}

void
Context::Done()
{
	assert(mClient != NULL);
	mClient->Done(this);
}

void
BaseClient::Close(bool wait)
{
	if (mStop) {
		return;
	}

	if (wait) {
		Wait();
	}

	mStop = true;
	Wakeup();
}

int
BaseClient::Put(Context* ctx, bool async)
{
	Mutex::Locker lock(mMutex);

	ctx->mClient = this;
	ctx->mUnique = Next();

	if (async) {
		mOutstanding.add(ctx->mUnique);
		lock.unlock();

		Send(ctx);

	} else {
		Send(ctx);
		mCond.wait_interval(mMutex, s_wait);
	}
    return 0;
}

void
BaseClient::Done(Context* ctx)
{
//	if (ctx->mAsync) {
#if OBJECT_PERFORM
	g_statis.inc(1, ctx->OccupySize() - object::c_object_head_size);
#else
	g_statis.inc(1, ctx->OccupySize());

	Mutex::Locker lock(mMutex);
	mOutstanding.del(ctx->mUnique);
	mCond.signal_all();
#endif
//	}
}

int
BaseClient::Wait(typeid_t unique)
{
	Mutex::Locker lock(mMutex);

	int ret = 0;
	while (Running()) {
		if (unique == -1) {
			if (mOutstanding.size() == 0) {
				break;
			}
		} else {
			if (!mOutstanding.get(unique)) {
				break;
			}
		}
		ret = mCond.wait_interval(mMutex, s_wait);
		if (ret != 0) {
			break;
		}
	}
	if (!Running()) {
		ret = -1;
	}
	return ret;
}

int
BaseClient::WaitOutstanding(size_t count)
{
	if (mOutstanding.size() <= count) {
		return 0;
	}

	Mutex::Locker lock(mMutex);

	int ret = 0;
	while (Running()) {
		if (mOutstanding.size() <= count) {
			break;
		}
		mCond.wait_interval(mMutex, s_wait);
	}
	if (!Running()) {
		ret = -1;
	}
	return ret;
}

void
BaseClient::Send(Context* ctx)
{
	GetWriter()->Put(ctx);
}

#include <sstream>
#include "Common/Display.hpp"

string
BaseConfig::DumpTest(const std::string& prefix, bool time)
{
	std::stringstream ss;
	ss << prefix;

	if (time) {
		ss << string_date() << "_" << string_time() << "_";
	}

	ss << string_count(test.total, false)
	   << "_" << (test.type == CT_put ? "put" : (test.type == CT_get ? "get" : "scan"))
	   << "_" << (data.uuid_key ? "uuid" : (data.rand_key ? "rand" : "seqn"))
	   << "_t[" << std::max(test.thread, 1) << "-" << test.batch << "]";

	if (data.key_max == data.key_min) {
		ss << "_k[" << string_size(data.key_max, false) << "]";
	} else {
		ss << "_k[" << string_size(data.key_min, false) << "-" << string_size(data.key_max, false) << "]";
	}

	if (data.data_max == data.data_min) {
		ss << "_d[" << string_size(data.data_max, false) << "]";
	} else {
		ss << "_d[" << string_size(data.data_min, false) << "-" << string_size(data.data_max, false) << "]";
	}

	//dump_reset();
//	ss << dump_last(", api(", ")")
//	   << dump_exist(!global.origin, "thrift");
	return ss.str();
}

