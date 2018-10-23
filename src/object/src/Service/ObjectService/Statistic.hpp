
#pragma once

#include "Perform/Statistic.hpp"
#include "Advance/Define.hpp"

#define OBJECT_STATIS(XX) 	\
	XX(writer, "")			\
	XX(reader, "")

#define OBJECT_TIMERS(XX) 	\
	XX(timer, "")

DEFINE_STATIS(ObjectStatis, OBJECT_STATIS);
DEFINE_TIMERS(ObjectTimers, OBJECT_TIMERS);

enum ReaderTimers {
	TS_null = 0,

	TS_create_table,
	TS_fetch_recover,
	TS_fetch_free,
	TS_commit_unit,
	TS_commit_object,
};

enum ReaderStatis {
	RS_null = 0,

	RS_drop,
	RS_ping_fail,

	RS_free,
	RS_used,
};

enum WriterStatus {
	WS_null = 0,

	WS_object_recv,
	WS_object_fail,
	WS_object_done,
	WS_object_retry,

	WS_object_size,
	WS_object_size_done,

	WS_recovr_recv,
	WS_recovr_done,

	WS_recovr_read,
	WS_recovr_read_failed,
	
	WS_recovr_trunc,
	WS_recovr_head_crash,
	WS_recovr_head_partial,
	
	WS_recovr_object,
	WS_recovr_object_head_crash,
	WS_recovr_object_trunc,

	WS_recovr_span,

};

