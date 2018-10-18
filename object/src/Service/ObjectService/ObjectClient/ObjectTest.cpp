
#include "ObjectTest.hpp"

#include <iostream>
#include <iomanip>
#include <unistd.h>

#include "Common/Define.hpp"
#include "Common/Logger.hpp"
#include "Common/Display.hpp"
#include "Common/File.hpp"
#include "Perform/StatThread.hpp"

#include "ObjectService/Control.hpp"
#include "ObjectService/Reader.hpp"

void
ClientTest::Notify(int type)
{
	mManage->Notify(mIndex, type);
}

void
ClientTest::Complete()
{
	mStatus = CS_done;
	mManage->Complete(mIndex);
}

void
ClientTest::Prepare()
{
	mStatus = CS_work;

	Notify(WT_started);

	//Restart();

	mSource.set_key(mConfig.data.rand_key, true, mConfig.data.uuid_key);
	mSource.set_data(true, mConfig.data.char_data, mConfig.data.uuid_data);

	mSource.key_range(mConfig.data.key_max, mConfig.data.key_min);
	mSource.data_range(mConfig.data.data_max, mConfig.data.data_min);

	Notify(WT_working);
}

void
ClientTest::Work(bool thread)
{
	if (thread) {
		success(mThread.create() == 0);
		return;
	}

	Prepare();

	switch(mConfig.test.type) {

	case BaseConfig::CT_put:{
		PutTest();
	} break;

	case BaseConfig::CT_get:{
	} break;

	case BaseConfig::CT_del:{
	} break;
	default:
		std::cout << "invalid test type " << mConfig.test.type << std::endl;
		assert(0);
		break;
	}

	Close();
	Complete();
}

void
ClientTest::PutTest()
{
	char* key;
	char* data;
	int klen, dlen;

	uint64_t total = mConfig.EachCount();
	for (uint64_t count = 0; count < total; count++) {
#if !OBJECT_PERFORM
		WaitOutstanding(mConfig.test.batch);
#endif
		mSource.key(key, klen);
		mSource.get(data, dlen);

		Context* ctx = Context::Malloc();
		ctx->Set(Context::OP_put);
		ctx->Data(key, klen, data, dlen);
		Put(ctx, true);
	}

	Close();
}

void
ClientManage::Start(BaseConfig& config)
{
	Configure(config);

	Preparing();

    int thread = 0;
	while (thread++ < mConfig.test.thread) {
		Add(mConfig);
	}

	Work();
}

void
ClientManage::Stop()
{
	g_statis_thread.stop();

	for (auto& client : mClients) {
		client.second->Stop();
		delete client.second;
	}
	mClients.clear();
}

void
ClientManage::Configure(BaseConfig& config)
{
	mConfig = config;

	g_statis.reset(mConfig.test.total);

	generator(mConfig.test.seed);
	common::tool::expect_char({{'0', '9'}, {'A', 'Z'}, {'a', 'z'}});

	mConfig.test.thread =
		std::max(mConfig.test.thread, 1);

	log_info(mConfig.DumpTest("test suit, "));
	log_info("");
}

void
ClientManage::Preparing()
{
	mCoordinator.total(mConfig.test.thread);
	mCoordinator.regist_array(WT_started, WT_end);

	log_info("preparing ...");

	PrepareObject();

	if (mConfig.test.recover) {
		main_loop();

	} else {
		if (mConfig.test.sleep) {
			log_info("sleeping ...");
			::usleep(mConfig.test.sleep * c_time_level[1]);
		}
		GetReader()->WaitReady();
		//g_statis_thread.start();
	}
}

void
ClientManage::PrepareObject()
{
	GetControl()->SetLogging();

	set_log_level(INFO, LOG_START);
	Reader reader(GetConfig());
	if (mConfig.test.clear) {
		TimeRecord time;
		std::cout << "  ...wait to clear: " << std::flush;

		if (reader.TestClearTable() != 0) {
			std::cout << std::endl;
			fault_format("preparing, clear table but failed, error\n%s", reader.Error().c_str());
		}
		std::cout << " table: " << string_record(time) << std::flush;

		::traverse_rmdir(GetConfig()->global.root);
		std::cout << ", data: " << string_record(time) << std::flush << std::endl;
	} else {
		if (reader.TestConnect() != 0) {
			fault_format("preparing, connect table but failed, error:\n%s", reader.Error().c_str());
		}
	}
	GetControl()->Start();
}

void
ClientManage::Work()
{
	loop_for_each_ptr(ClientTest, client, mClients)  {
		client->Work(true);
	}
	/** wait all client connected */
	Wait(WT_started);

	/** wait all client completed */
	Wait(WT_complete);

	WaitComplete();
}

void
ClientManage::WaitComplete()
{
	g_statis_thread.stop();

	int wait_time = 20000;

	#if OBJECT_PERFORM
		wait_time = 0;
	#endif
	/** wait for commit */
	GetControl()->Stop(wait_time);

	Stop();

	if (mConfig.test.wait) {
		::usleep(mConfig.test.wait * c_time_level[1]);
	}
}

void
ClientManage::Complete(int index)
{
	IOStatic* stat = g_statis_thread.statis();
	uint64_t count = mConfig.test.total - (stat->iops.total() + stat->iops.count());
	log_info("complete, index " << index << ", remain client " << mCoordinator.remain(WT_complete) - 1
		<< ", count " << count);

	Notify(index, WT_complete);
}

void
ClientManage::Notify(int index, int type)
{
	/** notify event count and wait, the last one will return true */
	if (mCoordinator.notify_wait(index, type)) {
		switch(type) {
			case WT_started: {
				log_info("****************************");
				log_info("connected ...");
				ResetStatis();
			}break;

#if 0
			case WT_prepare: {
				log_info("");
				log_info("----------------------------");
				log_info("pre-scanned ...");
				ResetStatis();
			}break;
#endif
			case WT_working: {
				log_info("----------------------------");

			}break;
			
            case WT_complete: {
			}break;

			default:{
				assert(0);
			}break;
		}
	}
}

void
ClientManage::ResetStatis()
{
	g_statis_thread.start();
	g_statis_thread.restart();
}

