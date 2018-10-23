
#include "Common/Logger.hpp"
#include "Common/ThreadPool.hpp"
#include "Common/CodeHelper.hpp"
#include "Advance/DynBuffer.hpp"

#include "Applet/Client/Command.hpp"
#include "Applet/Client/ClientBase.hpp"
#include "Applet/Client/ConfigBase.hpp"
#include "Applet/Client/ClientManage.hpp"

using namespace applet::client;

class ClientRequest;
class ClientImpl;

/**
 * define how to send package to server
 **/
void DispatchMessage(ClientRequest* request);

/**
 * client request, contain request data and context pointer
 **/
class ClientRequest
{
public:
	ClientRequest() {}
	virtual ~ClientRequest() { assert(mContext == NULL); }

public:
	/**
	 * set request info
	 **/
	void	Set(Context* context, common::Data* key, common::Data* data) {
		mContext = context;
		memcpy(mKey, key->ptr, std::min(key->len, (int)sizeof(mKey)));
		mData.write(data->ptr, data->len);
	}

	Context* DetachContext() {
		Context* context = mContext;
		mContext = NULL;
		return context;
	}

public:
	typedef common::DynBuffer Buffer;
	Context* mContext = NULL;

	ClientImpl* mImpl = NULL;
    common::byte_t mKey[256] = {0};
	Buffer mData;
};

/**
 * client interface wrapper
 **/
class ClientImpl : public Interface
{
public:
	ClientImpl(ClientBase* client) : Interface(client) {}

public:

	virtual int	Send(Context* context) {
		switch(context->mType) {

			case OP_put:
			default: {
				ClientRequest* request = new ClientRequest();
				request->Set(context, mClient->Key(), mClient->Data());

                DispatchMessage(request);
			} break;
//			default:{
//				assert(0);
//			}break;
		}
        return 0;
	}

public:
	/**
	 * request complete
	 **/
	void	Done(ClientRequest* request) {
		Context* context = request->DetachContext();
		context->Done((length_t)strlen(request->mKey), (length_t)request->mData.length());

		delete request;
	}
};

/**
 * server implement
 **/
class ServerImpl
{
public:
	typedef common::ThreadPool::Task Task;

	class ServerTask : public Task
	{
	public:
		ServerTask(void* param) { mRequest = (ClientRequest*)param; }

	public:
		void 	Set(ClientRequest* request) {
			mRequest = request;
		}

		virtual bool operator() () {
		#if 0
			usleep(1);
		#endif
			ClientImpl* impl = mRequest->mImpl;
			impl->Done(mRequest);
			mRequest = NULL;
            return true;
		}

	public:
		ClientRequest* 	mRequest = {NULL};
	};

public:
	int		Start() {
		mPool.manage(new common::TypeTaskManage<ServerTask>);
		return mPool.start(1);
	}

	int		Stop() {
		return mPool.stop();
	}

	int		Add(ClientRequest* request) {
		return mPool.add(request);
	}

protected:
    common::ThreadPool mPool;
};
SINGLETON(ServerImpl, ServerInstance)

void
DispatchMessage(ClientRequest* request)
{
	ServerInstance().Add(request);
}

int
main(int argc, char* argv[])
{
	Config config;
	Command command(&config);
	command.parse(argc, argv);
	std::cout << config.dump("base:\t") << std::endl;
	ServerImpl& server = ServerInstance();

	server.Start();
	ClientManage manage;
	manage.Start(&config, new TypeInterfaceFactory<ClientImpl>());

	server.Stop();
	return 0;
}

