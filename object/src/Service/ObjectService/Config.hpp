
#pragma once

#include <string>
using std::string;

/** perform test */
#define OBJECT_PERFORM	1
#if OBJECT_PERFORM
#	define WRITE_TIME			10000
#	define VIRTUAL_MACHINE		1
#	include "Perform/Timer.hpp"
#endif

#include "Common/Const.hpp"
#include "ObjectService/PostgreSQL.hpp"

struct ObjectConfig
{
	const ObjectConfig& operator = (const ObjectConfig& v) {
		global 	= v.global;
		writer 	= v.writer;
		reader 	= v.reader;
		table	= v.table;
		return *this;
	}

	struct Global {
		/** root path */
		string	root	= {"/var/object"};
		/** server id */
		int		sid 	= { 0 };
		/** dump thread wait time */
		int		dump 	= {1000};
	} global;

	struct Writer {
		/** writer thread */
		int		thread	= { /*5*/ 2 };
		/** unit size */
		int		unit = { 64 * c_length_1M };

		struct Task {
			int		wait	= {1000};
			int 	retry	= {3};
		} task;

		struct IO {
			bool	sync	= {false};
			bool	direct	= {/*true*/ false};
		} io;

	} writer;

	struct Reader {

		/** db connect conn */
		PostConnInfo conn;

		struct Recover {
			/** concurrent recover limit */
			int		concurrent = {5};
			/** prepare count before work */
			int		step = {3};
		} recover;

		struct {
			int		min = {5};
			int		max = {10};
		} free;

		struct Interval {
			/** thread wait time */
			int		thread	= { 1000 };
			/** recover time */
			int		recover = { 5000 };
			/** ping wait interval */
			int		ping	= { 10000 };
			/** commit object interval */
			int		object	= { 300 };
			/** commit unit interval */
			int		unit	= { 1000 };
			/** fetch unit wait */
			int		fetch 	= {50};
		} interval;

		struct Commit {
			int		limit	= { c_length_64K };
			int		batch	= { c_length_1K };
			int		unit	= { 5 };
		} commit;

	} reader;

	struct Table {
		string	allocation	= { "allocation" };
		/** index, server_id, status */
		string	unit_table	= { "unit" };
		/** name, index, offset, status */
		string	object_table = { "object" };
		/** temp object table */
		string	object_new	 = { "object_new" };
		/** sid, time */
		string	server_table = { "server" };

		string	index	= { "index" };
		string	curr	= { "current" };
		string	sid 	= { "sid" };
		string	old_sid	= { "old_sid"};
		string	status	= { "status" };
		string	time 	= { "time" };

		string	name	= { "name" };
		/** offset is key-word in pg */
		string	offset  = { "off" };
		int		timeout	= { 60000 };

	} table;
};

enum ObjectErrorStadge
{
	OS_null = 0,
	OS_stopping,
	OS_alloc_unit,

	OS_unit_open,
	OS_unit_read,
	OS_unit_write,
	OS_unit_seek,
	OS_unit_stat,
	OS_unit_trunc,
	OS_unit_flush,
	OS_unit_close,

	OS_object_write,
	OS_object_data,
	OS_object_commit,

	OS_done = OS_null
};



