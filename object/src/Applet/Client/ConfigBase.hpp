
#pragma once

#include <vector>
#include <string>

#include "Common/Const.hpp"

namespace applet {
namespace client {

	typedef std::vector<int64_t> array_t;

	/**
	 * default config
	 **/
	struct Config
	{
	public:
		Config() {}

		Config(const Config& v) {
			operator = (v);
		}

		Config& operator = (const Config& v) {
			help	= v.help;
			host	= v.host;
			load	= v.load;
			work 	= v.work;
			data	= v.data;
			return *this;
		}

	public:
		/**
		 * dump config status
		 **/
		virtual std::string dump(const std::string& prefix = "", bool time = false);

		/**
		 * dump base config info
		 **/
		std::string	dump_item(int type);

		/**
		 * get work count for each thread
		 **/
		uint64_t	EachCount() {
			return load.total / load.thread;
		}

	public:

		struct Help {
			/** config file path, read config if exist */
			std::string path = {""};
			/** wait before start */
			int		sleep 	= {0};
			/** wait before stop */
			int		wait 	= {1};
			/** record config source */
			std::string source;
		} help;

		/**
		 * host definition
		 **/
		struct Host {
			/** remote host */
			std::vector<std::string> addrs;
			/** remote port */
			uint16_t	port = {0};
		} host;

		/**
		 * workload
		 **/
		struct Load {
			/** each node thread count */
			int		thread	= {0};
			/** default total count */
			int64_t	total	= {100000};
			/** each client batch count */
			int     batch	= {1};
		} load;

		/**
		 * work type
		 **/
		struct Work {
			/** clear data */
			bool	clear	= {false};
			/** just wait and run as service */
			bool	server = {false};
			/** random seed */
			typeid_t seed	= {0};
			/** default test type */
			int		type	= {0};
			/** work type range */
			array_t	type_ranges;
			/** work type range weight */
			array_t	type_weight;
			/** request wait timeout */
			int		wait	= {30000};
			/** output interval */
			int		interval = {0};
		} work;

		/**
		 * data range
		 **/
		struct Data {
			/** rand or sequnce */
			bool	rand_key 	= {true};
			/** random data */
			bool	rand_data	= {true};
			/** use char as key */
			bool	char_key 	= {true};
			/** readable */
			bool	char_data	= {false};
			/** using uuid key */
			bool	uuid_key	= {false};
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
			/** data size range */
			array_t	ranges;
			/** data size range weight */
			array_t	weight;
		} data;

	protected:
		enum Type {
			T_help = 0,
			T_host,
			T_load,
			T_work,
			T_data,
			T_type_range,
			T_data_range,
		};
	};

}
}

