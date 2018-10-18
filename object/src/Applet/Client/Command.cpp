
#include <iostream>
#include <fstream>
#include <exception>

#include "Common/Type.hpp"
#include "Common/String.hpp"
#include "Common/Display.hpp"

#include "Applet/Client/Command.hpp"
#include "Applet/Client/ConfigBase.hpp"

using namespace common;

namespace applet {
namespace client {

std::string
Config::dump(const std::string& prefix, bool time)
{
	std::stringstream ss;
	ss << prefix;
	if (time) {
		ss << string_date() << "_" << string_time() << "_";
	}

	if (work.server) {
		ss << "work as daemon server";

	} else {
		ss  << dump_item(T_load)
			<< "\t Host" << dump_item(T_host)
			<< "\n" << dump_item(T_type_range)
			<< "\n" << dump_item(T_data_range)
			<< "\t" << dump_item(T_help);
	}
	return ss.str();
}


std::string
Config::dump_item(int type)
{
	std::stringstream ss;
	switch (type) {
	case T_help: {
		if (help.source != "") {
			ss << help.source << ": '"<< help.path << "'";
		}
	} break;
	case T_host: {
		if (host.addrs.size()) {
			ss << "[";
			for (size_t i = 0; i < host.addrs.size(); i++) {
				ss << host.addrs[i];
				if (i != host.addrs.size() - 1) {
					ss << ", ";
				}
			}
			ss << (host.port != 0 ? (" port: " + n2s(host.port)) : "") << "]";}

	} break;
	case T_load:
	case T_work:
	case T_data: {
		ss << (work.clear ? "clear_" : "")
		   << "T" << work.type
		   << "_" << string_count(load.total, false)
		   << "_" << (data.uuid_key ? "uuid" : (data.rand_key ? "rand" : "seqn"))
		   << "_" << (work.seed != 0 ? ("S" + n2s(work.seed)) : "")
		   << "_thread[" << std::max(load.thread, 1) << "-" << load.batch << "]";

		if (data.key_max == data.key_min) {
			ss << "_k[" << string_size(data.key_max, false) << "]";
		} else {
			ss << "_k[" << string_size(data.key_min, false) << "-" << string_size(data.key_max, false) << "]";
		}
		if (data.data_max == data.data_min) {
			ss << "_d[" << string_size(data.data_max, false) << "]";
		} else {
			ss << "_d[" << string_size(data.data_min, false) << "-" << string_size(data.data_max, false) << "]";
		}
	} break;
	case T_type_range: {
		if (work.type_ranges.size()) {
			int64_t total = 0;
			for (auto v : work.type_weight) {
				total += v;
			}
			ss << "type:\t";
			for (size_t index = 0; index < work.type_weight.size(); index++) {
				if (work.type_weight[index] > 0) {
					ss << "[" << work.type_ranges[index] << ":" << string_percent(work.type_weight[index], total) << "], ";
				}
			}
		}
	} break;
	case T_data_range: {
		if (data.ranges.size()) {
			int64_t total = 0;
			for (auto v : data.weight) {
				total += v;
			}
			ss << "size:\n";
			for (size_t index = 0; index < data.weight.size(); index++) {
				if (data.weight[index] > 0) {
					ss << "\t[" << string_size(data.ranges[index], false)
					   << "-" << string_size(data.ranges[index + 1], false) << ") : \t"
					   << string_percent(data.weight[index], total) << "\n";
				}
			}
		}
	} break;
	default:
		assert(0);
		break;
	}
	return ss.str();
}

void
single_range(const std::string& value, int& v_min, int& v_max)
{
	auto fix_unit = [](int& v, std::string& str) {
		v = string_integer<int>(str);
		/** set as invalid data */
		if (v == 0 && str.length() == 0) {
			v = -1;
		}
	};

	std::string s1, s2;
	common::split(value, s1, s2);

	int max, min;
	fix_unit(min, s1);
	fix_unit(max, s2);

	v_min = std::min(max, min);
	v_max = std::max(max, min);
	if (v_min == -1) {
		v_min = v_max;
	}
}

void
array_range(const std::string& value, array_t& array, bool ascend)
{
	std::string str;
	std::string output = value;

	int64_t last = -1;
	int64_t curr = 0;

	while ((str = common::parse(output)).length() != 0) {
		curr = string_integer<int64_t>(str);
		if (ascend && last > curr) {
			throw std::logic_error("array " + value + " should ascending");
		}
		last = curr;
		array.push_back(curr);
	}
}

/**
 * 1,2,3
 * 10,10, 10
 **/
void
type_ranges(const std::string& _sr, const std::string& _sw, array_t& ranges, array_t& weight)
{
	std::string sr = _sr;
	std::string sw = _sw;

	array_range(sr, ranges, true);
	array_range(sw, weight, false);

	int64_t last = -1;
	for (auto curr : ranges) {
		if (last >= curr) {
			throw std::logic_error("parse type range " + sr + " failed, type bound should never equal");
		}
		last = curr;
	}
	ranges.push_back(*ranges.rbegin() + 1);

	if (ranges.size() != weight.size() + 1) {
		if (!distill(sr, sr, "[", "]")) {
			throw std::logic_error("type range " + sr + " size " + n2s(ranges.size() - 1)
				+ ", weight " + sw + " size " + n2s(weight.size())
				+ ", type range size should equal weight size");
		}
	}
}

/**
 * 1,2,3
 * [1,2,2,3,8,100]
 **/
void
size_ranges(const std::string& _sr, const std::string& _sw, array_t& ranges, array_t& weight)
{
	std::string sr = _sr;
	std::string sw = _sw;
	bool interval = false;

	/** use [] as seperator */
	if (common::find(sr, "[]")) {
		if (!distill(sr, sr, "[", "]")) {
			throw std::logic_error("parse range " + sr + " failed, '[' and '] should appear simultaneously");
		}
		interval = true;
	}
	array_range(sr, ranges, true);
	array_range(sw, weight, false);

	if (interval) {
		array_t temp;
		for (auto curr : weight) {
			temp.push_back(curr);
			temp.push_back(0);
		}
		temp.resize(temp.size() - 1);
		weight = temp;

		for (size_t index = 0; index + 1 < ranges.size(); index += 2) {
			if (ranges[index] == ranges[index + 1]) {
				throw std::logic_error("parse range " + sr + " failed, range bound should not equal in a piece");
			}
		}

	} else {
		int64_t last = -1;
		for (auto curr : ranges) {
			if (last >= curr) {
				throw std::logic_error("parse range " + sr + " failed, range bound should never equal");
			}
			last = curr;
		}
	}

	size_t rs = interval ? ranges.size() / 2 : ranges.size() - 1;
	size_t ws = interval ? (weight.size() + 1) / 2 : weight.size();
	if (rs != ws) {
		throw std::logic_error("range " + sr + " size " + n2s(rs)
			+ ", weight " + sw + " size " + n2s(ws)
			+ ", range size should equal weight size");
	}
}

const char*
Command::validate_name(const char* name)
{
	std::string::size_type pos = std::string(name).rfind(".");
	if (pos != std::string::npos) {
		name = name + pos + 1;
	}
	return name;
}

void
Command::conflict(const std::string& opt1, const std::string& opt2)
{
    if (m_varmap.count(opt1) && !m_varmap[opt1].defaulted()
        && m_varmap.count(opt2) && !m_varmap[opt2].defaulted())
    {
        throw std::logic_error("conflicting options '"
        	+ opt1 + "' with '" + opt2 + "'.");
    }
}

void
Command::depend(const std::string& for_what, const std::string& required_option)
{
    if (m_varmap.count(for_what) && !m_varmap[for_what].defaulted()) {
        if (m_varmap.count(required_option) == 0 || m_varmap[required_option].defaulted()) {
            throw std::logic_error("Option '" + for_what
                 + "' requires option '" + required_option + "'.");
        }
    }
}

/**
 * usage:
 * command line or command file:
 * 			[--config path | --config=path | -c path | -cpath ]
 * 			 --host 1.1.2.2 192.2.3.4
 *
 * configure file:
 * 			uuid_key = 1/0/true/false
 *          total    = 1000000
 *          host     = 1.1.2.2.
 *          host     = 192.2.3.4
 *
 * more options: required(), multitoken(), composing(), implicit_value(), default_value, notifier
 **/
void
Command::default_options()
{
	Config& config = *m_config;
	m_option.add_options()
	//m_help.add_options()
		("help,h", 		"help")
		("config,c",  	PO_TYPE(config.help.path), "configure file as ini config")
		("file,f",  	PO_TYPE_EMPTY(config.help.path), "command file keep command")
		("sleep",		PO_TYPE(config.help.sleep), "sleep before start")
		("wait",		PO_TYPE(config.help.wait), 	"wait when complete\n")
	//m_host.add_options()
		/** allow multi host input, separated by space */
		("host",  		PO_TYPE_EMPTY(config.host.addrs)->multitoken()->composing(), "remote host")
		("port", 		PO_TYPE(config.host.port), "host port")

	//m_work.add_options()
		("clear", 		PO_BOOL(config.work.clear), "clear all data")
		("type", 		PO_TYPE(config.work.type), "message type")
		("server", 		PO_BOOL(config.work.server), "run as daemon server")
		("interval,i", 	PO_TYPE(config.work.interval), "output interval\n")

	//m_load.add_options()
		("thread", 		PO_TYPE(config.load.thread), "thread count")
		("total,t", 	PO_TYPE(config.load.total),  "total count")
		("batch,b", 	PO_TYPE(config.load.batch),  "batch request\n")

	//m_data.add_options()
		("seqn_key", 	"sequence key")
		("seqn_data", 	"sequence data")
		("any_key", 	"not use only readable char as key")
		("char_data", 	PO_BOOL(config.data.char_data), "use char as data")
		("uuid_key", 	PO_BOOL(config.data.uuid_key), "uuid key")
		("uuid_data", 	PO_BOOL(config.data.uuid_data), "uuid data")
		("key_size,k", 	po::value<std::string>()->default_value("40"), "key size range")
		("data_size,d", po::value<std::string>()->default_value("4K,8K"), "data size range")
		("seed", 		PO_TYPE(config.work.seed), "default 0 for random seed")
		("typer", 		po::value<std::string>(), "type ranges 1,3,4")
		("typew", 		po::value<std::string>(), "type ranges weight 10,10,20")
		("ranges", 		po::value<std::string>(), "data size ranges 1,3,4,5 or [1,3, 3,4, 4,5]")
		("weight", 		po::value<std::string>(), "data size ranges weight 10,10,20, less than range by 1");

	//m_opts.add(m_help).add(m_work).add(m_load).add(m_data);
	//m_opts.add(m_help).add(m_load).add(m_data);

	/** first positional option will be file */
	m_position.add("file", 1);
}

void
Command::check_conflict()
{
	conflict("config", "file");
	conflict("seqn_key", "uuid_key");

	depend("port", "host");
	depend("ranges", "weight");
	depend("typer", "typew");
}

void
Command::parse(int argc, char* argv[])
{
	try {
		default_options();

		parse_command(argc, argv);

		parse_configure();

		check_conflict();

		default_parse();

	} catch(std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        exit(-1);

    } catch(...) {
    	std::cerr << "Exception of unknown type!\n";
    	exit(-1);
    }
}

void
Command::parse_command(int argc, char* argv[])
{
	//po::store(po::parse_command_line(argc, argv, m_option), m_varmap);
	po::store(po::command_line_parser(argc, argv).
	          options(m_option).positional(m_position).run(), m_varmap);
	po::notify(m_varmap);
}

void
Command::parse_configure()
{
	Config& config = *m_config;
	std::string path = config.help.path;
	if (path.empty()) {
		return;
	}

	if (set(path, "file")) {
		std::ifstream ifs(path);
		if (!ifs) {
			throw std::logic_error("can not open command file: " + path);

		} else {
			std::string line;
			std::string str;
			while (!ifs.eof()) {
				std::getline(ifs, str);
				line += " " + str;
			}
			config.help.source = "command file";

		    std::vector<std::string> array = po::split_unix(line);
			po::store(po::command_line_parser(array).options(m_option).run(), m_varmap);
			po::notify(m_varmap);
		}

	/** check config file */
	} else if (set(path, "config")) {
		std::ifstream ifs(path);

		if (!ifs) {
			/** not set config option, config file not exist */
			//if (m_varmap["config"].defaulted()) {
			//	return;
			//}
			throw std::logic_error("can not open config file: " + path);

		} else {
			/** not set config option, default config file exist */
			if (m_varmap["config"].defaulted()) {
				std::cout << "using default config file '"
						<< path << "'" << std::endl;
			}
			config.help.source = "configure";

			po::store(po::parse_config_file(ifs, m_option), m_varmap);
			po::notify(m_varmap);
		}
	}
}

void
Command::help()
{
	//std::cout << m_opts << std::endl;
	m_option.print(std::cout);
	exit(-1);
}

void
Command::default_parse()
{
	Config& config = *m_config;
	if (m_varmap.count("help")) {
		help();
	}

	PO_REV_BOOL(*this, config.data.rand_key, "seqn_key");
	PO_REV_BOOL(*this, config.data.rand_data, "seqn_data");
	PO_REV_BOOL(*this, config.data.char_key, "any_key");

	std::string value;
	if (set(value, "key_size")) {
		single_range(value,
			config.data.key_min,
			config.data.key_max);
	}
	if (set(value, "data_size")) {
		single_range(value,
			config.data.data_min,
			config.data.data_max);
	}

	std::string weight;
	if (set(value, "typer") && set(weight, "typew")) {
		type_ranges(value, weight, config.work.type_ranges, config.work.type_weight);
	}
	if (set(value, "ranges") && set(weight, "weight")) {
		size_ranges(value, weight, config.data.ranges, config.data.weight);
	}

	if (config.load.thread > 1024) {
		std::cerr << "set thread as " << config.load.thread
			 << ", try -t again "<< std::endl;
		exit(-1);
	}
}

}
}

