
#pragma once

#include "Perform/Mock.hpp"

enum ObjectWaitType {

	WT_null = 10000,

	/** reader event */
	WT_prepare_table,
	WT_fetch_recover,
	WT_fetch_free,
	WT_recover_done,

	WT_commit_object,
	WT_commit_object_wait,
	WT_commit_object_batch,
	WT_commit_object_wakeup,
	WT_commit_object_failed,

	WT_commit_unit,
	WT_commit_unit_wait,
	WT_commit_unit_batch,
	WT_commit_unit_wakeup,
	WT_commit_unit_failed,

	WT_fetch_unit_wait,
	WT_fetch_unit_tout,
	WT_notify_unit_emtpy,

	WT_restart,
	WT_loop,

	WT_drop,
	WT_drop_trigger,

	/** ping success */
	WT_ping,
	/** ping failed */
	WT_ping_failed,

	WT_test_recover,
	/** writer event */
	WT_object_done,
	WT_unit_roll,
	WT_unit_trunc,

	WT_unit_head_partial,
	WT_unit_head_crash,
	WT_unit_parse_head,
	WT_unit_write_head,

	WT_object_trunc,
	WT_object_parse,
	WT_object_head_crash,
	WT_object_block_crash,
	WT_object_length_exceed,

	WT_object_write_head,
	WT_object_write_data,

	WT_end
};

