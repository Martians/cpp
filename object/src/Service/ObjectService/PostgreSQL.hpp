
#pragma once

#include <string>
#include <postgresql/libpq-fe.h>

#include "Common/Time.hpp"
#include "Advance/Define.hpp"
#include "Advance/AppHelper.hpp"

using std::string;

/**
 * postgresql connect info
 **/
struct PostConnInfo
{
	string		host = {"127.0.0.1"};
	uint16_t	port = {0};
	string		user = {"postgres"};
	string		passwd = {"111111"};
	uint32_t	conn_time = {0};

	string		dbname = {"mydb"};
	string 		application;
};

/**
 * format postgresql connect info
 **/
inline PostConnInfo
SetConnInfo(const string& host, const string& user, const string& passwd,
	const string& dbname, uint32_t conn_time = 0)
{
	PostConnInfo info;
    info.host = host;
    info.port = 0;
    info.user = user;
    info.passwd = passwd;
    info.dbname = dbname;
    info.conn_time = conn_time;
	return info;
}

#define DataBaseMap(XX)							\
	XX(DE_connect_noroute, "No route to host")	\
	XX(DE_connect_refuse, "Connection refused")	\
	XX(DE_connect_timeout, "timeout expired")	\
	XX(DE_connect_closed, "connection closed")	\
	XX(DE_auth_denay, "authentication failed")	\
	XX(DE_duplicate_key, "duplicate key value")	\
	XX(DE_already_exist, "already exists")		\
	XX(DE_not_exist, "does not exist")			\
	XX(DE_empty, "empty command")

DEFINE_ENUM(DataBaseError, DataBaseMap);

/**
 * postgresql wrapper
 **/
class PostgreSQL
{
public:
	PostgreSQL() {}
	virtual ~PostgreSQL() { Close(); }

public:
	/**
	 * connect to postgresql
	 **/
	int		Connect(PostConnInfo* info);

	/**
	 * close current connection
	 **/
	void	Close();

	/**
	 * execute command
	 */
	int		Execute(const char* fmt, ...);

	/**
	 * prepare for append message
	 **/
	void	Prepare(const char* fmt, ...);

	/**
	 * append more execute messsage
	 **/
    void	Append(const char* fmt, ...);

    /**
     * execute pending string
     **/
	int		Execute(bool clear = true) {
		int ret = DoExecute(mCommand, true);
		if (clear) {
			mCommand.clear();
		}
		return ret;
	}

	/**
	 * drop table
	 **/
	int		DropTable(const string& table, bool cascade = false);

public:
	/**
	 * ignore error
	 **/
	void	IgnoreError(int error) { mIgnore = error; }

	/**
	 * ignore error
	 **/
	void	MuteError(int error) { mMute = error; }

	/**
	 * check if error match given type
	 *
	 * @note check error from result message
	 **/
	bool	CurrError(int type);

	/**
	 * get error key
	 **/
	const char* ErrorString(int type);

	/**
	 * get last error code
	 * @note get info from PQstatus(mPGconn) and PQresultStatus(mResult)
	 **/
	int		Errno() { return mCurr.Errno(); }

	/**
	 * get last error message
	 * @note get info from PQerrorMessage(mPGconn) and PQresultErrorMessage(mResult)
	 **/
    const std::string& Error() { return mCurr.Error(); }

	/**
	 * get last result for later use
	 **/
	PGresult*	Result() { return mResult; }

	/**
	 * get cached command
	 **/
	std::string& Command() { return mCommand; }

	/**
	 * last execute time
	 **/
	TimeRecord&	Time() { return mLast; }

	/**
	 * last execute time
	 **/
	ctime_t		LastTime() { return mLast.last(); }

	/**
	 * get cached command
	 **/
	void 		SwapCommand(string& command) {
		mCommand.swap(command);
	}

	/**
	 * get last string result
	 **/
	const char* SingleString(int i = 0, int j = 0);

	/**
	 * get last int32_t result
	 **/
	int32_t	SingleInt32(int i = 0, int j = 0);

	/**
	 * get last int64_t result
	 * @note last command must have only one result
	 **/
	int64_t	SingleInt64(int i = 0, int j = 0);

	/**
	 * dump table all data
	 **/
	int		DumpTable(const string& table, uint32_t limit = -1);

	/**
	 * dump connection status
	 **/
	bool	DumpConnStatus();

	/**
	 * dump result status
	 **/
	void	DumpResultStatus();


	/**
	 * postgresql event handle
	 **/
	int		EventHandle(int event, void *evtInfo);

	/**
	 * for debug
	 **/
	int		MockDrop();

protected:
    /**
     * for debug
     * */
    int     Socket() { return PQsocket(mPGconn); }

	/**
	 * check if error ignored
	 **/
	int		IgnoreError();

	/**
	 * mute with registed error
	 **/
	int		MuteError();

	/**
	 * set last error code and message
	 **/
	int		Error(int code, const string& message = "") {
		if (code != 0 && message.length() == 0) {
			const char* errmsg = ErrorString(DE_connect_closed);
			return mCurr.Error(code, errmsg);

		} else {
    		return mCurr.Error(code, message);
        }
		//mCurr.mString.resize(mCurr.mString.length() - 1);
	}

	/**
	 * set last error code and message
	 **/
	void	Error(int code, int param, const string& message = "") { mCurr.Error(code, param, message); }

	/**
	 * clear connection
	 **/
	void	ClearConnect() {
		if (mPGconn) {
			PQfinish(mPGconn);
			mPGconn = NULL;
		}
	}

	/**
	 * clear result
	 **/
	void	ClearResult() {
		if (mResult) {
			PQclear(mResult);
			mResult = NULL;
		}
	}

	/**
	 * reset error
	 **/
	void	ClearError() { mCurr.Reset(); }

	/**
	 * clear current info
	 **/
	void	ClearCurrent() {
		mLast.reset();
		ClearResult();
		ClearError();
	}

protected:
	/**
	 * check if connection is ready, if not, will try new connection
	 **/
	int		RetryConn();

	/**
	 * actually execute command
	 * @param command string
	 * @param retry inner use for keep retry count, in case dead-loop
	 **/
	int		DoExecute(const string& command, bool retry);

	/**
	 * check server state
	 **/
	int		PingServer(PostConnInfo* info);

protected:
	PostConnInfo mConnInfo;
	PGconn*		mPGconn = {NULL};
	PGresult*	mResult = {NULL};
	int			mIgnore = {0};
	int			mMute	= {0};
	string		mCommand;
	TimeRecord	mLast;
	CurrentStatus mCurr;
};

#if 0
const struct _SQLEnd{}	SQLEnd;

inline PostgreSQL&
operator << (PostgreSQL& pg, const string& value) {
	pg.Append(value);
	return pg;
}

inline int
operator << (PostgreSQL& pg, const _SQLEnd& value) {
	return pg.Execute();
}
#endif

