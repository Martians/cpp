
#define LOG_CODE 0

#include <sstream>
#include <cassert>
#include <cstdarg>
#include <postgresql/libpq-events.h>

#include "Common/Display.hpp"
#include "Common/String.hpp"
#include "Advance/Container.hpp"

#include "ObjectService/LogReader.hpp"
#include "UnitTest/Mock.hpp"
#include "ObjectService/PostgreSQL.hpp"

RegistMap<int, string> s_errmap = INITIAL_MAP(DataBaseMap);

string
ConnString(PostConnInfo* info)
{
	//postgresql://[user[:password]@][netloc][:port][/dbname][?param1=value1&...]
	std::stringstream ss;
	ss << "postgresql://" << info->user << ":" << info->passwd << "@" << info->host;
	if (info->port) {
		ss << ":" << info->port;
	}
	ss << "/" << info->dbname << "?connect_timeout=" << info->conn_time;

	if (info->application.length()) {
	   ss << "&application_name=" << info->application;
	}
	return ss.str();
}

static int
EventHandle(PGEventId evtId, void *evtInfo, void *passThrough)
{
	PostgreSQL* pg = (PostgreSQL*)passThrough;
	pg->EventHandle((int)evtId, evtInfo);
    return 1;
}

int
PostgreSQL::EventHandle(int event, void *evtInfo)
{
	return 0;

	const char* value = "";
	switch (event) {
	case PGEVT_CONNRESET:		value = "CONNRESET";
		break;
	case PGEVT_CONNDESTROY:		value = "CONNDESTROY";
		break;
	case PGEVT_REGISTER:		value = "REGISTER";
		break;
	case PGEVT_RESULTCREATE:	value = "RESULTCREATE";
		break;
	case PGEVT_RESULTCOPY:		value = "RESULTCOPY";
		break;
	case PGEVT_RESULTDESTROY:	value = "RESULTDESTROY";
		break;
	default:
		break;
	}
	log_info("###### recv event " << value);
    return 0;
}


#include<sys/socket.h>
#include <unistd.h>

int
PostgreSQL::MockDrop()
{
	int fd = Socket();
	if (fd != 0) {
		::shutdown(fd, SHUT_RD);
		return 0;
	}
    return -1;
}

int
PostgreSQL::Connect(PostConnInfo* info)
{
	Close();
	mConnInfo = *info;

	string conninfo = ConnString(&mConnInfo);
	mPGconn = PQconnectdb(conninfo.c_str());
	assert(mPGconn != NULL);

	/** regist event */
	PQregisterEventProc(mPGconn, ::EventHandle, "event handle", this);

	if (PQstatus(mPGconn) != CONNECTION_OK) {
		Error(PQstatus(mPGconn), PQerrorMessage(mPGconn));
		MockWakeupH(Errno(), DumpConnStatus());
		log_warn("Connect to database failed: " << Error());

	} else {
		log_debug("connect on " << mConnInfo.host);
	}
	return Errno();
}

void
PostgreSQL::Close()
{
	ClearCurrent();

	ClearConnect();
}

int
PostgreSQL::RetryConn()
{
	if (!mPGconn) {
		return -1;
	}
	/** current state is good, no need retry */
	if (PQstatus(mPGconn) == CONNECTION_OK) {
		//log_trace("retry conn, there is no need");
		return -1;

	} else if (MockDecData(WT_drop_trigger)) {
		//log_trace("retry conn, but mock wt_drop now");
		return -1;
	}

	PQreset(mPGconn);
	/** retry success */
	if (PQstatus(mPGconn) == CONNECTION_OK) {
		log_debug("reset connection with " << mConnInfo.host);
		return 0;
	}
	return -1;
}

int
PostgreSQL::Execute(const char* fmt, ...)
{
	FORMAT_SIZE(c_length_64K, fmt);
	return DoExecute(data, true);
}

void
PostgreSQL::Prepare(const char* fmt, ...)
{
	FORMAT_SIZE(c_length_64K, fmt);
	mCommand = data;
}

void
PostgreSQL::Append(const char* fmt, ...)
{
	FORMAT_SIZE(c_length_64K, fmt);
	mCommand += data;
}

int
PostgreSQL::IgnoreError()
{
	if (mIgnore != 0) {
		/** ignore all errors */
		if (mIgnore == -1) {
			return 0;

		} else if (CurrError(mIgnore)) {
			return 0;
		}
	}
	return -1;
}

int
PostgreSQL::MuteError()
{
	if (mMute != 0) {
		/** ignore all errors */
		if (mMute == -1) {
			return 0;

		} else if (CurrError(mMute)) {
			return 0;
		}
	}
	return -1;
}

bool
PostgreSQL::CurrError(int type)
{
	return common::find_exact(Error(), s_errmap.get(type));
}

const char*
PostgreSQL::ErrorString(int type)
{
	return s_errmap.get(type).c_str();
}

int
PostgreSQL::DoExecute(const string& command, bool retry)
{
	if (command.length() == 0) {
		log_trace("execute command, ignore empty command");
		return Error(DE_empty, ErrorString(DE_empty));
	}
	ClearCurrent();

	mResult = PQexec(mPGconn, command.c_str());
	mLast.check();
	ExecStatusType status = PQresultStatus(mResult);
	if (status != PGRES_COMMAND_OK && 
		status != PGRES_TUPLES_OK)
	{
		/** if connect failed, re-connect */
		if (retry && RetryConn() == 0) {
			return DoExecute(command, false);
		}
		Error(status, PQresultErrorMessage(mResult));

		if (IgnoreError() == 0) {
			debug("config ingore as " << mIgnore << " - " << string_timer(mLast)
				<< ", pass error " << Error());
			Error(0);

		} else if (MuteError() != 0) {
			//PQresStatus(status) -> PGRES_FATAL_ERROR
			log_debug("execute command, '" << command << "' - " << string_timer(mLast)
				<< ", but failed:\n" /*<< ", status " << PQresStatus(status)*/ << Error());
        }
		ClearResult();

	} else {
		trace("execute: '" << command << "' - " << string_timer(mLast));
	}
	return Errno();
}

const char*
PostgreSQL::SingleString(int i, int j)
{
	return PQgetvalue(mResult, i, j);
}

int32_t
PostgreSQL::SingleInt32(int i, int j)
{
	return s2n<int64_t>(SingleString());
}

int64_t
PostgreSQL::SingleInt64(int i, int j)
{
	return s2n<int64_t>(SingleString());
}

int
PostgreSQL::DropTable(const string& table, bool cascade)
{
	return Execute("DROP TABLE %s %s", table.c_str(), cascade ? "CASCADE" : "");
}

int
PostgreSQL::PingServer(PostConnInfo* info)
{
	PGPing status = PQping(ConnString(info).c_str());
	switch (status) {
	case PQPING_OK:			return 0;
		break;
	case PQPING_REJECT:
		break;
	case PQPING_NO_RESPONSE:
		break;
	case PQPING_NO_ATTEMPT:
		break;
	default:
		assert(0);
		break;
	}
	return -1;
}

bool
PostgreSQL::DumpConnStatus()
{
	// PGTransactionStatusType PQtransactionStatus(mPGconn)
	log_debug("dump stat: " << PQuser(mPGconn) << "@" << PQhost(mPGconn) << ":" << PQport(mPGconn)
		<< ", conn " << PQstatus(mPGconn) << " pid " << PQbackendPID(mPGconn) << ", db " << PQdb(mPGconn));
	return true;
}

void
PostgreSQL::DumpResultStatus()
{
	log_debug("dump result: command " << PQcmdStatus(mResult)
		<< ", effect " << PQcmdTuples(mResult) << " rows");
}

int
PostgreSQL::DumpTable(const string& table, uint32_t limit)
{
	Execute("select * from %s limit %d", table.c_str(), limit);
	DumpConnStatus();

    string display;
	int field = PQnfields(mResult);
	for (int i = 0; i < field; i++) {
		append(display, "%-15s", PQfname(mResult, i));
	}
	log_debug(display);

	for (int i = 0; i < PQntuples(mResult); i++) {
        display.clear();
		for (int j = 0; j < field; j++) {
			append(display, "%-15s", PQgetvalue(mResult, i, j));
		}
	    log_debug(display);
	}
	return 0;
}

