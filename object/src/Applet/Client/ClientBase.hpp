
#pragma once

#include "Common/Cond.hpp"
#include "Common/Container.hpp"
#include "Common/DataType.hpp"
#include "CodeHelper/RunStat.hpp"
#include "Perform/Source.hpp"

#include "Applet/Client/Interface.hpp"
#include "Applet/Client/ConfigBase.hpp"

namespace applet {
namespace client {

	class ClientBase;
	class ClientManage;

	/**
	 * context container
	 **/
	class Context
	{
	public:
		Context() {}
		virtual ~Context() {}

	public:
		/**
		 * set context info
		 **/
		void	Set(int type, typeid_t unique, ClientBase* client) {
			mType 	= type;
			mUnique = unique;
			mClient = client;
		}

		/**
		 * request done, update
		 **/
		void  	Done(int64_t key, int64_t data);

	public:
		/** request type */
		int		mType	= {0};
		/** async request */
		bool	mAsync  = {false};
		/** unique id */
		typeid_t mUnique = {0};
		/** error code */
		int		mError	= {0};
		/** client handle*/
		ClientBase*	mClient = {NULL};
	};

	/**
	 * client worker
	 * */
	class ClientBase
	{
	public:
		ClientBase(ClientManage* manage, int index, Config* config);
		virtual ~ClientBase();

		enum Stauts
		{
			CS_null = 0,
			CS_work,
			CS_done,
		};

	public:
		/**
		 * set client option
		 **/
		void	Set(Interface* impl, Config* config = NULL) { 
            mImpl   = impl ? impl : mImpl;
            mConfig = config ? config : mConfig; 
        }

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
		void	Stop();

	public:
		/**
		 * generate key
		 **/
		struct common::Data* Key(bool update = true, int specific = 0) {
			if (update) {
				mSource[0]->get(&mData[0], specific);
			}
			return &mData[0];
		}

		/**
		 * generate data
		 **/
		struct common::Data* Data(bool update = true, int specific = 0) {
			if (update) {
				if (mRanges[1] && specific == 0) {
					specific = (int)mRanges[1]->next_piece();
				}
				mSource[1]->get(&mData[1], specific);
			}
			return &mData[1];
		}

		/**
		 * create new context
		 **/
		Context* NextContext(int type);

		/**
		 * increase statistic value
		 **/
		void  	Statis(int64_t key, int64_t data);

		/**
		 * send message out
		 **/
		int		SendOut(Context* ctx);

		/**
		 * get data
		 **/
		int		Get(Context* ctx);

		/**
		 * delete data
		 * */
		int		Del(Context* ctx);

		/**
		 * wait until done or timeout
		 **/
		int		Wait(typeid_t type = -1);

		int		WaitOutstanding(size_t count);

		/**
		 * wakeup notify
		 **/
		void	Wakeup() {
            common::Mutex::Locker lock(mMutex);
			mCond.signal_all();
		}

		/**
		 * request done
		 **/
		void	Done(Context* ctx);

		/**
		 * next unique
		 **/
		typeid_t Next() { return ++mUnique; }

	protected:

		class Thread;

		RUNSTATE_DEFINE;

		/**
		 * prepare for test
		 * */
		void	Prepare();

		/**
		 * waiting for start
		 **/
		void	Notify(int type);
		/**
		 * test complete
		 **/
		void	Complete();

		/**
		 * do mutator test
		 **/
		void	Message();

		/**
		 * get client id
		 **/
		int		Index() { return mIndex; }

		/**
		 * connect with object
		 **/
		void	Restart() {}

		/**
		 * close remote
		 **/
		bool	Close(bool wait = true);

		/**
		 * request context
		 **/
		void 	Send(Context* ctx);

		friend class ClientManage;

	protected:
		/** client manage pointer */
		ClientManage* mManage;
		/** client thread index */
		int			mIndex	= {0};
		/** global config */
		Config*		mConfig = {NULL};
		/** local thread */
		Thread* 	mThread = {NULL};
		/** client interface */
		Interface* 	mImpl = {NULL};
		/** client status */
		int			mStatus	= {CS_null};

		/** data generator */
		source_ptr mSource[2];
		/** range generator */
		random_ptr mRanges[2];
		/** temporary buffer */
        struct common::Data	mData[2];

		common::Mutex mMutex = {"client", true};
        common::Cond  mCond;
        common::TypeSet<typeid_t> mOutstanding;
		typeid_t mUnique = {0};
	};
}
}

