
#include "Common/Display.hpp"
#include "Common/Thread.hpp"
#include "Common/Logger.hpp"
#include "Advance/TypeAlloter.hpp"
#include "Perform/StatThread.hpp"
#include "Common/DataType.hpp"
#include "Applet/Client/ClientManage.hpp"
#include "Applet/Client/ClientBase.hpp"

namespace applet {
namespace client {

static ::common::TypeAlloter<Context> s_context(4096);

void
Context::Done(int64_t key, int64_t data)
{
	assert(mClient != NULL);
	mClient->Done(this);

	g_statis.inc(1, key + data, mError);
}

class ClientBase::Thread : public common::CommonThread
{
public:
	Thread(ClientBase* client) : mClient(client) {}

public:

	virtual void* entry() {
		mClient->Work();
		return NULL;
	}

protected:
	ClientBase*	mClient;
};

ClientBase::ClientBase(ClientManage* manage, int index, Config* config)
	: mManage(manage), mIndex(index), mConfig(config)
{
}

ClientBase::~ClientBase()
{
	Close();

	if (mThread) {
		mThread->join();
		reset(mThread);
	}
}

void
ClientBase::Work(bool thread)
{
	if (thread) {
		if (change_status(true)) {
			if (!mThread) {
				mThread = new Thread(this);
				success(mThread->create() == 0);
			}
		}
		return;
	}

	Prepare();

	Message();

	Close();

	Complete();
}

void
ClientBase::Stop()
{
	Close(false);

	if (mThread) {
		mThread->join();
		reset(mThread);
	}
}

void
ClientBase::Prepare()
{
	mStatus = CS_work;
	Notify(ClientManage::WT_started);

	using namespace ::common::random;
	mSource[0] = SourceFactory::get(Source::Param(Source::parse_type(mConfig->data.rand_key, mConfig->data.uuid_key),
			mConfig->data.key_min, mConfig->data.key_max, mConfig->data.char_key),
			/** if use seqn_key, should keep all seqn source have the same start */
			Random::Param(mConfig->work.seed + (mConfig->data.rand_key ? mIndex : 0)));

	/** use sequence key, should reset range */
	if (!mConfig->data.rand_key) {
		rand_t step = mConfig->EachCount();
		std::dynamic_pointer_cast<SeqnSource>(mSource[0])->increase(mIndex * step);
	}

	mSource[1] = SourceFactory::get(Source::Param(Source::parse_type(mConfig->data.rand_data, mConfig->data.uuid_data),
			mConfig->data.data_min, mConfig->data.data_max, mConfig->data.char_data), Random::Param(mConfig->work.seed * mIndex));

	/** use type piecewise range */
	if (mConfig->work.type_ranges.size()) {
		Random::Param param(mConfig->work.seed * mIndex);
		param.range(mConfig->work.type_ranges, mConfig->work.type_weight);
		mRanges[0] = RandomFactory::get(param);
	}
	/** use data piecewise range */
	if (mConfig->data.ranges.size()) {
		Random::Param param(mConfig->work.seed * mIndex);
		param.range(mConfig->data.ranges, mConfig->data.weight);
		mRanges[1] = RandomFactory::get(param);
	}
	int ret = 0;
	if ((ret = mImpl->Start(mConfig)) != 0) {
		fault_string("client index %d, prepare connect but failed,\n%s", common::syserr(ret).c_str());
	}

	Notify(ClientManage::WT_working);
}

void
ClientBase::Notify(int type)
{
	mManage->Notify(mIndex, type);
}

void
ClientBase::Complete()
{
	mStatus = CS_done;
	mManage->Complete(mIndex);
}

void
ClientBase::Message()
{
#if PIECEWISE_OUTPUT
	using namespace ::common::random;
	execute(mRanges[0], set_bit(Random::T_record, 1));
	execute(mRanges[1], set_bit(Random::T_record, 1));
#endif

#if RANDOM_OUTPUT
	std::stringstream ss;
#endif
	uint64_t total = mConfig->EachCount();
	for (uint64_t count = 0; count < total; count++) {
		WaitOutstanding(mConfig->load.batch);

		/** use piecewise typer */
		Context* context = NextContext(mRanges[0] ?
			mRanges[0]->next_piece() : mConfig->work.type);
		SendOut(context);

#if RANDOM_OUTPUT
		{
			if (count == 0 || count == total - 1) {
				ss << " " << Key()->ptr;
				if (count == total - 1) {
					log_info(mIndex << "-" << ss.str());
				}
			}
		}
		#if 0
		std::stringstream ss;
		ss << " " << Key()->ptr;
		log_info(mIndex << "-" << ss.str());
		#endif
#endif
	}
	/** wait all task completed */
	WaitOutstanding(0);

#if PIECEWISE_OUTPUT
	execute(mRanges[0], dump_piece());
	execute(mRanges[1], dump_piece());
#endif
}

bool
ClientBase::Close(bool wait)
{
	if (!change_status(false)) {
		return false;
	}

	if (wait) {
		Wait();
	}

	mImpl->Close();
	Wakeup();

	return true;
}

Context*
ClientBase::NextContext(int type)
{
	Context* ctx = s_context.get();
	ctx->Set(type, Next(), this);
	return ctx;
}

int
ClientBase::SendOut(Context* ctx)
{
    common::Mutex::Locker lock(mMutex);

	if (ctx->mAsync) {
		mOutstanding.add(ctx->mUnique);
		lock.unlock();
		mImpl->Send(ctx);

	} else {
		mImpl->Send(ctx);
		mCond.wait_interval(mMutex, mConfig->work.wait);
	}
    return 0;
}

void
ClientBase::Done(Context* ctx)
{
    common::Mutex::Locker lock(mMutex);
	mOutstanding.del(ctx->mUnique);
	mCond.signal_all();
	lock.unlock();

	s_context.put(ctx);
}

int
ClientBase::Wait(typeid_t unique)
{
    common::Mutex::Locker lock(mMutex);

	int ret = 0;
	while (running()) {
		if (unique == -1) {
			if (mOutstanding.size() == 0) {
				break;
			}
		} else {
			if (!mOutstanding.get(unique)) {
				break;
			}
		}
		ret = mCond.wait_interval(mMutex, mConfig->work.wait);
		if (ret != 0) {
			break;
		}
	}
	if (!running()) {
		ret = -1;
	}
	return ret;
}

int
ClientBase::WaitOutstanding(size_t count)
{
    common::Mutex::Locker lock(mMutex);

	int ret = 0;
	while (running()) {
		if (mOutstanding.size() <= count) {
			break;
		}
		ret = mCond.wait_interval(mMutex, mConfig->work.wait);
	}
	if (!running()) {
		ret = -1;
	}
	return ret;
}

void
ClientBase::Statis(int64_t key, int64_t data)
{
	g_statis.inc(1, key + data);
}

}
}
