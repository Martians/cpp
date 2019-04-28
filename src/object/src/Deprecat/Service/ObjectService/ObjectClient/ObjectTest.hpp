
#pragma once

#include "Perform/Source.hpp"
#include "Common/Thread.hpp"
#include "Common/Container.hpp"
#include "Advance/AppHelper.hpp"
#include "Advance/Coordinator.hpp"
#include "ObjectClient.hpp"

enum ClientStauts
{
	CS_null = 0,
	CS_work,
	CS_done,
};

enum WaitType
{
	WT_null = 0,
	/** thread start */
	WT_started,
	/** try to work */
	WT_working,
	/** prepare done */
	WT_prepare,
	/** complete work */
	WT_complete,
	WT_end
};

class ClientManage;

/**
 * client interface
 * */
class ClientTest : public BaseClient
{
public:
	ClientTest(int index, BaseConfig& config, ClientManage* manage = NULL)
		: BaseClient(index, config), mThread(this), mManage(manage) {}

	virtual ~ClientTest() {}

public:
	/**
	 * get status
	 **/
	int		Status() { return mStatus; }

	/**
	 * do test work
	 **/
	void	Work(bool thread = false);

	/**
	 * stop current client
	 **/
	void	Stop() {
		Close(false);
		mThread.join();
	}

protected:

	class ClientThread : public common::CommonThread
	{
	public:
		ClientThread(ClientTest* client) : mClient(client) {}

	public:

		virtual void* entry() {
			mClient->Work();
			return NULL;
		}
	protected:
		ClientTest*	mClient;
	};

	/**
	 * waiting for start
	 **/
	void	Notify(int type);
	/**
	 * test complete
	 **/
	void	Complete();

	/**
	 * prepare for test
	 * */
	void	Prepare();

	/**
	 * do mutator test
	 **/
	void	PutTest();

protected:
	/** client status */
	int		mStatus	= {CS_null};
	/** data generator source */
	DataSource 	  mSource;
	ClientThread  mThread;
	ClientManage* mManage;
};

class ClientManage
{
public:
	ClientManage() {}
	virtual ~ClientManage() { Stop(); }

public:
	/**
	 * add new client config
	 **/
	ClientTest*	Add(BaseConfig& config) {
		ClientTest* client = new ClientTest(mUnique.next(), config, this);
		ClientTest** pclient = mClients.add(client->Index(), &client);
		return *pclient;
	}

	/**
	 * start all client
	 **/
	void	Start(BaseConfig& config);

	/**
	 * stop all client
	 **/
	void	Stop();

	/**
	 * try to work
	 **/
	void	Work();

	/**
	 * client event complete
	 **/
	void	Notify(int index, int type);

	/**
	 * wait event
	 **/
	void	Wait(int type) {
		mCoordinator.wait(type);
	}

	/**
	 * client complete
	 **/
	void	Complete(int index);

protected:

	ClientTest* GetClient(int index) {
		ClientTest** pclient = mClients.get(index);
		assert(pclient != NULL);
		return *pclient;
	}

	/**
	 * fix config
	 **/
	void	Configure(BaseConfig& config);

	/**
	 * prepare env
	 **/
	void	Preparing();

	void	PrepareObject();
	void	WaitComplete();

	/**
	 * reset statistic
	 **/
	void	ResetStatis();

protected:
	TypeMap<int, ClientTest*> mClients;
	BaseConfig mConfig;
	Coordinator	mCoordinator;
	Unique<int> mUnique;
};
