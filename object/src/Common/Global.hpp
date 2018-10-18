
#pragma once

namespace common {

	typedef void (*void_handle_t)();

	/**
	 * regist init work
	 **/
	void	regist_init(void_handle_t handle);

	/**
	 * regist exit work
	 **/
	void 	regist_exit(void_handle_t handle);

	/**
	 * do all init work
	 **/
	void 	initialize();

	/**
	 * do all exit work
	 **/
	void 	exit();
}
