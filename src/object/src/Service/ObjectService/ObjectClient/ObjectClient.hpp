
#pragma once

#include <string>
using std::string;

#include "Common/Container.hpp"
#include "Common/Cond.hpp"
#include "Service/ObjectService/Object.hpp"

using namespace common;

class BaseClient;

struct BaseConfig
{
public:
	BaseConfig() {}

	BaseConfig(const BaseConfig& v) {
		operator = (v);
	}

	const BaseConfig& operator = (const BaseConfig& v) {
		test 	= v.test;
		data	= v.data;
		return *this;
	}

public:

	string		DumpTest(const std::string& prefix = "", bool time = false);

	uint64_t	EachCount() {
		return test.total / test.thread;
	}

public:
	enum Type {
		CT_put = 0,
		CT_get,
		CT_del,
	};

	struct Test {
		/** random seed */
		int		sleep 	= { 0 };
		int		wait 	= { 1 };
		int		seed	= {0};
		bool	clear	= {false};
		bool	recover	= {false};
		int		thread	= {0};
		/** default test type */
		int		type	= { CT_put };
		/** default total count */
		int64_t	total	= { /*1000000*/ 3 };
		/** each client batch count */
		int     batch	= { 1 };
	} test;

	struct Data {
		/** rand or sequnce */
		bool	rand_key 	= {true};
		/** using uuid key */
		bool	uuid_key	= {false};
		/** readable */
		bool	char_data	= {false};
		/** using uuid data */
		bool	uuid_data	= {false};
		/** key min */
		int		key_min		= {-1};
		/** key max */
		int		key_max		= {40};
		/** data min */
		int		data_min	= {-1};
		/** data max */
        int		data_max	= {c_length_4K};
	} data;
};

/**
 * request context
 **/
class Context : public Object
{
public:
	Context(int type = OP_null, ObjectUsr* usr = NULL, Container* container = NULL)
		: mType(type) {}

    virtual ~Context() {}
public:
	enum Operation {
		OP_null = 0,
		OP_put,
		OP_get,
	};

public:
	void	Set(int type, byte_t* usr = NULL, uint32_t ulen = 0,
		byte_t* container = NULL, uint32_t clen = 0)
	{
		mType = type;

		SetUser(usr, ulen);
		SetContainer(container, clen);
	}

	void	Data(byte_t* key, uint32_t klen,
		byte_t* data, uint32_t dlen)
	{
		SetKey(key, klen);
		SetData(data, dlen);
	}

	/**
	 * malloc new context
	 **/
	static Context* Malloc();

	/**
	 * cycle context
	 **/
	virtual void Cycle();

	/**
	 * request done
	 **/
	virtual void Done();

public:
	/** request type */
	int		mType = {OP_null};
	/** protocol type */
	typeid_t mApp = {0};

	/** unique id */
	typeid_t mUnique = {0};
	/** async request */
	bool	mAsync  = { false };
	/** request client */
	BaseClient*	mClient = {NULL};
};

/**
 * client interface
 * */
class BaseClient
{
public:
	BaseClient(int index, BaseConfig& config) : mConfig(config), mIndex(index) {}

	virtual ~BaseClient() { Close(false); }

public:
	/**
	 * set client option
	 **/
	void	Set(const BaseConfig& config) { mConfig = config;
	}

	/**
	 * get client id
	 **/
	int		Index() { return mIndex; }

	/**
	 * connect with object
	 **/
	void	Restart() {}

	void	Close(bool wait = true);


	/**
	 * request context
	 **/
	void 	Send(Context* ctx);

public:
	/**
	 * put data
	 **/
	int		Put(Context* ctx, bool async = false);

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
		Mutex::Locker lock(mMutex);
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

	bool	Running() { return !mStop; }

protected:
	BaseConfig mConfig;
	TypeSet<typeid_t> mOutstanding;
	typeid_t mUnique = {0};
	Mutex	mMutex = {"client", true};
	bool	mStop  = {false};
	Cond	mCond;
	int		mIndex	= {0};
};


