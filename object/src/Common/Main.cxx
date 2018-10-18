
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <string>

#include "Common/Global.hpp"
#include "Common/Display.hpp"
#include "Perform/Debug.hpp"

namespace common {
namespace tester {

	class HandleArray
	{
	public:
		HandleArray() {}

		typedef void (*simple_handle_t)();
		typedef simple_handle_t handle_t;

	public:
		/**
		 * regist handle
		 **/
		void	Regist(int index, simple_handle_t handle, const char* name) {
			array [index] = handle;
			string[index] = name;
		}

		/**
		 * get handle
		 **/
		handle_t Get(int index) { return array[index]; }

		/**
		 * set current index
		 **/
		void	Current(int index) { current = index; }

		/**
		 * get current index
		 **/
		int		Current() { return current; }

		/**
		 * work the index
		 **/
		void	Work(int index) {
			if (Get(index)) {

				TimeRecord time;
				std::cout << "  ...testing [" << index << "] " << std::left << std::setw(20) << string[index] << std::flush;
				Get(index)();
				prints("using: " << string_record(time));
			}
		}

		/**
		 * dump help
		 **/
		void	Dump() {
			prints("test command help:");
			for (int i = 0; i < c_handle_size; i++) {
				if (array[i]) {
					prints("\t<" << i << "    " << string[i]);
				}
			}
		}
		static const int c_handle_size = 100;

	protected:
		handle_t array [c_handle_size] = {};
		std::string 	string[c_handle_size] = {};
		int 	index	= {0};
		int		current = {0};
	};
	static HandleArray g_array;

	void
	check_param(int argc, char* argv[])
	{
		char ch;
		opterr = 0;
		while ((ch = getopt(argc, argv, "h")) != -1) {
			switch(ch) {
			case 'h': {
				g_array.Dump();
				::exit(0);
			} break;
			default:
				break;
			}
		}

		if (argc > 1) {
			int input = atoi(argv[1]);
			if (input > 0 && input < HandleArray::c_handle_size) {
				assert(g_array.Get(input) != NULL);
				g_array.Current(input);
			}
		}

		if (g_array.Current() == 0) {
			for (int i = 0; i < HandleArray::c_handle_size; i++) {
				g_array.Work(i);
			}
		} else {
			g_array.Work(g_array.Current());
		}
	}

	#define REGIST(index, handle)	\
			void handle();			\
			g_array.Regist(index, handle, #handle)

	void regist_handle()
	{
		REGIST(1, logger_test);
		REGIST(2, time_thread_test);
		REGIST(3, thread_pool_test);
		REGIST(4, mem_pool_test);
		REGIST(5, dyn_buffer_test);
		REGIST(6, base_alloter_test);
		REGIST(7, seqn_source_test);
		REGIST(8, rand_source_test);
		REGIST(9, barrier_test);
		REGIST(10, convert_test);
		REGIST(11, statistic_test);
		REGIST(12, random_test);
	}
}
}
using namespace common;
using namespace common::tester;

int
main(int argc, char* argv[])
{
	regist_handle();
	check_param(argc, argv);

	thread_wait();
    return 0;
}

