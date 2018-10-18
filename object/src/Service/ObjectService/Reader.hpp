
#pragma once

#include "ObjectService/Config.hpp"
#include "ObjectService/Object.hpp"
#include "ObjectService/PostgreSQL.hpp"
#include "Common/Container.hpp"
#include "Common/TypeQueue.hpp"
#include "Common/ThreadBase.hpp"

using namespace common;

/**
 * unit    : [index] server [status time]
 * location: [name ] index  off
 **/
class Reader : public common::ThreadBase
{
public:
	Reader(ObjectConfig* config);
	virtual ~Reader() { Stop(); }

	/**
	 * reader status
	 **/
	enum Status
	{
		ST_null = 0,
		ST_prepare,
		ST_recover,
		ST_work,
		ST_drop,
		ST_stop,
	};

	/**
	 * unit status in reader
	 **/
    enum UnitStatus {
		US_recovr = 0,
		US_update,
		US_free,
		US_using,
		US_commit,

		US_max
    };

    /**
     * unit timer
     **/
    enum UnitTimer {
		TM_ping = 0,
		TM_drop,

		TM_recovr,
		TM_object,
		TM_unit,

		TM_max,
    };

public:
    /**
     * drop local work
     **/
	void	Dropped();

	/**
	 * recover to work again
	 **/
	void	Restart();

	/**
	 * start work
	 **/
	void	Start();

	/**
	 * stop work
	 **/
	void	Stop(int wait = 0);

	/**
	 * commit unit
	 **/
	virtual int	CommitUnit(const UnitIndex& index, bool recover = false);

	/**
	 * alloc unit
	 **/
	virtual int	FetchUnit(UnitIndex& index, int wait);

	/**
	 * finish unit
	 */
	virtual int CommitObject(Object* object);

	/**
	 * recover work done
	 **/
	virtual int	RecoverDone(const UnitIndex& index, bool success);

	/**
	 * get object location
	 **/
	int     Get(const string& object, Location& location);

	/**
	 * del object
	 **/
	void	Del(const string& object);

	/**
	 * get current error
	 **/
	const std::string& Error() { return mDB.Error(); }
    
	/**
	 * wait until reader is ready
	 **/
	void	WaitReady();

public:
	/**
	 * dump inner status
	 **/
	string& DumpStatus(string& str);

	/**
	 * dump current unit
	 **/
	string	DumpUnit(int type);

    /**
     * check if we can connect to remote
     * */
    int     TestConnect() { return Connect(); }

    /**
     * clear inner table, be careful
     **/
    int		TestClearTable();

protected:
    /**
     * thread process
     **/
    virtual void *entry();

    /**
     * check current status
     **/
    bool	Disabled() { return Status(ST_drop) || Status(ST_stop); }

    /**
     * do unit work
     **/
    void	UnitWork();

    /**
     * do object work
     **/
    void	ObjectWork();

    /**
	 * prepare for database
	 **/
	int		Prepare();

    /**
     * connect to remote
     **/
    int		Connect() { return mDB.Connect(&mConfig->reader.conn); }

	/**
	 * prepare table
	 **/
	int		PrepareTable();

	/**
	 * create object table
	 **/
	int		CreateObjectTable(bool new_one = false);

	/**
	 * regist local server
	 **/
	int		RegistServer(int sid);

	/**
	 * update server info
	 **/
	int		PingServer();

	/**
	 * execute ping
	 **/
	int		ExecutePing();

    /**
     * recover un-committed unit
     **/
	int 	RecoverUnit();

	/**
	 * format fetch recover
	 **/
	int		FetchRecover();

	/**
	 * need recover more unit or not
	 **/
	bool	RecoverMoreUnit();

    /**
     * send writer to recover unit file
     **/
    int		WriterRecover(const UnitIndex& index, int32_t sid);

    /**
     * get recover count
     **/
    int64_t RecoverCount();

	/**
	 * get more unit
	 **/
	int     FetchUnit();

    /**
     * fetch more free unit or not
     **/
    bool	FetchMoreUnit();

	/**
	 * commit unit
	 **/
	void 	CommitUnit();

	/**
	 * format update unit
	 **/
	void	UpdateUnit(int type);

    /**
     * check if need commit unit
     **/
    bool	TryCommitUnit(bool check = false);

	/**
	 * commit object index
	 **/
	void	CommitObject();

    /**
     * check if need commit object
     **/
    bool	TryCommitObject(bool check = false);

	/**
	 * query inner object
	 **/
	int     QueryObject();

    /**
     * get upsert string
     **/
    const std::string& Upsert();

protected:
	/**
	 * reset timer config
	 **/
	void	ResetTimer();

	/**
	 * check if timer is timeout
	 **/
	bool	Timeout(int type, bool check = false) {
		return check ? mWait[type].check() : mWait[type].expired();
	}

	/**
	 * update timer
	 **/
	void	UpdateTime(int type) {
		mWait[type].reset();
	}

	/**
	 * update timer
	 **/
	void	WakeupTime(int type) {
		mWait[type].wakeup();
	}

	size_t	UnitCount(int type) {
		return mUnit[type].size();
	}

	/**
	 * wait commit object count
	 **/
    size_t	WaitCommit() {
    	return mList.size() + mRetryList.size() + mTemp.size();
    }

    /**
	 * wait commit unit count
	 **/
    size_t	WaitCommitTotal() {
    	return UnitCount(US_commit) + UnitCount(US_update) + WaitCommit();
    }

	/**
	 * get writer config
	 **/
	ObjectConfig& GlobalConfig() { return *mConfig; }

    /**
     * get reader config
     **/
    ObjectConfig::Reader& Config() {
		return mConfig->reader;
	}

	/**
	 * get table config
	 **/
    ObjectConfig::Table& Table() {
        return mConfig->table;
    }

    /**
     * check current status
     **/
    bool	Status(int status) { return mStatus == status; }

    /**
     * wakeup work
     **/
    void	WakeupWork();

protected:
    /**
     * record recover status
     **/
    class WaitRecover {
    public:
    	void	Reset() {
    		time.reset();
    		work	= false;
    		curr	= 0;
    		wait	= 0;
    		commit 	= 0;
    		object 	= 0;
    	}
    public:
        TimeRecord time;
        /** recover status */
        bool	work = {false};
        /** last recover count */
        int64_t curr = {0};
        /** wait recover count */
        int64_t wait = {0};

        /** for record */
        int64_t	commit = {0};
        int64_t	object = {0};
    };

    /**
	 * set wait condition before work status
	 **/
	void	SetWaitWork(int count);

	/**
	 * check if can start work
	 **/
	bool	TryWork();

	/**
	 * trigger work condition
	 **/
	void	ReadyWork(bool wakeup);

protected:
    ObjectConfig* mConfig = {NULL};
    TypeQueue<Object*> mList;
    TypeSet<UnitIndex> mUnit[US_max];
    TimeCheck	mWait[TM_max];
    PostgreSQL 	mDB;

    WaitRecover mWaitRecovr;
    TypeQueue<Object*> mTemp;
    TypeQueue<Object*> mRetryList;
    /** retry command */
    std::string mRetryCommand;
    size_t	mRetryLength;

    Cond	mEmptyCond;
    Cond	mTaskCond;
    int		mStatus	 = {ST_null};
    int64_t mPingFail  = {0};
    ThreadBase mThread;
};
