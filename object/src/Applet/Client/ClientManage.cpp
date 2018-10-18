
#include <iostream>
#include <iomanip>
#include <unistd.h>

#include "Common/Define.hpp"
#include "Common/Logger.hpp"
#include "Common/Display.hpp"
#include "Perform/Source.hpp"
#include "Perform/StatThread.hpp"

#include "Applet/Client/ClientManage.hpp"

using namespace common;

namespace applet {
namespace client {

ClientManage::~ClientManage()
{
	Stop();

	reset(mFactory);
	reset(mEvent);
}

void
ClientManage::Start(Config* config,
	InterfaceFactory* factory,
	EventHandle* event)
{
	mFactory = factory;
	mEvent = event ? event : new EventHandle;

	Configure(config);

	Preparing();

    int thread = 0;
	while (thread++ < mConfig->load.thread) {
		AddClient();
	}

	Work();
}

void
ClientManage::Stop()
{
	g_statis_thread.stop();

	for (auto& client : mClients) {
		client.second->Stop();

		mFactory->Cycle(client.second->mImpl);
		client.second->mImpl = NULL;
		delete client.second;
	}
	mClients.clear();
}

void
ClientManage::Configure(Config* config)
{
	mConfig = config;

	/** set total count for output */
	g_statis.reset(mConfig->load.total);

	/** update seed */
	common::random::update_seed(mConfig->work.seed);

	if (mConfig->data.char_key) {
		common::random::chared_table().expected({{'0', '9'}, {'A', 'Z'}, {'a', 'z'}});
	}

	mConfig->load.thread =
		std::max(mConfig->load.thread, 1);

	log_info(mConfig->dump("test suit, "));
	log_info("");
}

void
ClientManage::Preparing()
{
	mCoordinator.total(mConfig->load.thread);
	mCoordinator.regist_array(WT_started, WT_end);

	PrepareEnv();

	if (mConfig->work.server) {
		log_info("local loop wait ...");
        common::main_loop();

	} else {
		if (mConfig->help.sleep) {
			log_info("sleeping ...");
			::usleep(mConfig->help.sleep * c_time_level[1]);
		}
	}
}

void
ClientManage::PrepareEnv()
{
	log_info("preparing ...");
	//set_log_level(INFO, LOG_START);

	int ret = 0;
	if (mConfig->work.clear) {
		TimeRecord time;

		std::cout << "  ...wait to clear: " << std::flush;
		if ((ret = mEvent->ClearEnv()) != 0) {
			fault_string("preparing, clear env but failed,\n%s", syserr(ret).c_str());
		}
		std::cout << ", data: " << string_record(time) << std::flush << std::endl;

	} else {
		if ((ret = mEvent->TestEnv()) != 0) {
			fault_string("preparing, test env but failed, \n%s", syserr(ret).c_str());
		}
	}
	if ((ret = mEvent->Start()) != 0) {
		fault_string("preparing, start service but failed, \n%s", syserr(ret).c_str());
	}
}

void
ClientManage::Work()
{
	for (auto client: mClients) {
		client.second->Work(true);
	}

	/** wait all client connected */
	Wait(WT_started);

	/** wait all client completed */
	Wait(WT_complete);

	Complete();
}

void
ClientManage::Complete()
{
	g_statis_thread.stop();

	mEvent->Close();

	Stop();
}

void
ClientManage::Complete(int index)
{
	IOStatic* stat = g_statis_thread.statis();
	uint64_t count = mConfig->load.total - (stat->iops.total() + stat->iops.count());
	log_info("complete, index " << index << ", remain client " << mCoordinator.remain(WT_complete) - 1
		<< ", count " << count);

	Notify(index, WT_complete);
}

void
ClientManage::Notify(int index, int type)
{
	log_trace("client " << index << ", notify " << type);

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

}
}
