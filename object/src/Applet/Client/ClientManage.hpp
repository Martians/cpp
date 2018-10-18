
#pragma once

#include "Common/Define.hpp"
#include "Common/Container.hpp"
#include "Advance/AppHelper.hpp"
#include "Advance/Coordinator.hpp"

#include "Applet/Client/ClientBase.hpp"
#include "Applet/Client/ConfigBase.hpp"

namespace applet {
namespace client {

	class ClientManage
	{
	public:
		ClientManage() {}
		virtual ~ClientManage();

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

	public:
		/**
		 * start all client
		 **/
		void	Start(Config* config, InterfaceFactory* factory, EventHandle* event = NULL);

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

		/**
		 * add new client
		 **/
		void    AddClient() {
			ClientBase* client = new ClientBase(this, mUnique.next(), mConfig);
			Interface* interface = mFactory->Malloc(client);
            client->Set(interface);
            success(mClients.add(client->mIndex, client));
		}

		/**
		 * fix config
		 **/
		void	Configure(Config* config);

		/**
		 * do prepare work
		 **/
		void	Preparing();

		/**
		 * perpare environment
		 **/
		void	PrepareEnv();

		/**
		 * work complete
		 **/
		void	Complete();

		/**
		 * reset statistic
		 **/
		void	ResetStatis();

	protected:
		Config*		mConfig = {NULL};
        common::TypeMap<int, ClientBase*> mClients;
		common::Coordinator	mCoordinator;
		common::Unique<int> mUnique;

		InterfaceFactory* mFactory = {NULL};
		EventHandle* mEvent = {NULL};
	};
}
}
