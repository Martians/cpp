
#pragma once

namespace applet {
namespace client {

	class ClientBase;
	class Config;
	class Context;

	/**
	 * operation type, you can extend more after OP_extend
	 **/
	enum Operation {
		OP_null = -1,
		OP_put  = 0,
		OP_get,
		OP_del,
		OP_extend
	};

	/**
	 * client communicate interface
	 **/
	class Interface
	{
	public:
		Interface(ClientBase* client)
			: mClient(client) {}
		virtual ~Interface() {}

	public:
		/**
		 * start connect to server
		 **/
		virtual int	Start(Config* config) { return 0; }
        
        /**
         * close connection
         **/
        virtual int Close() { return 0; }

        /**
         * send message to remote
         **/
		virtual	int	Send(Context* ctx) = 0;

	protected:
		ClientBase*	mClient = {NULL};
	};

	/**
	 * interface factory, each client thread will get an interface
	 **/
	class InterfaceFactory
	{
	public:
		virtual ~InterfaceFactory() {}

		/**
		 * malloc new interface
		 **/
		virtual Interface* Malloc(ClientBase* client) = 0;

		/**
		 * cycle interface
		 **/
		virtual void Cycle(Interface* impl) = 0;
	};

	/**
	 * interface factory simple helper
	 **/
	template<class Type>
	class TypeInterfaceFactory : public InterfaceFactory
	{
	public:
		virtual Interface* Malloc(ClientBase* client) { return new Type(client); }

		virtual void Cycle(Interface* impl) { delete (Type*)impl; }
	};

	/**
	 * event handle for
	 **/
	class EventHandle
	{
	public:
		virtual ~EventHandle() {}

		/**
		 * clear environment before start, if set clear flag
		 **/
		virtual int	ClearEnv() { return 0; }

		/**
		 * test if environment in good state, if failed, will abort
		 **/
		virtual int	TestEnv() { return 0; }

		/**
		 * start global background service for interface if needed
		 **/
		virtual int Start() { return 0; }

		/**
		 * all task complete, stop and recycle
		 **/
		virtual int Close() { return 0; }
	};
}
}
