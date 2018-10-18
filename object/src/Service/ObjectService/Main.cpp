
#include <boost/program_options.hpp>
#include <boost/algorithm/String.hpp>
#include <iostream>
#include <string>

#include "Common/Type.hpp"
#include "Common/Logger.hpp"
#include "Common/String.hpp"
#include "Common/Display.hpp"

#include "ObjectService/Config.hpp"
#include "ObjectService/Control.hpp"
#include "ObjectClient/ObjectTest.hpp"

using namespace std;
namespace po = boost::program_options;

string
next_unit(const string& str)
{
	return " \n \b\b\b\b\b\b\b\b\b\b\b\b\b\b [" + str + "]";
}

void
help(po::options_description& opdesc)
{
	std::cout << opdesc << endl;
	exit(-1);
}

void
fix_unit(int& v, std::string& s)
{
	int unit = 1;
	if (s.length() >= 2) {
		std::string::size_type pos;;
		if ((pos = s.find_first_of("Kk"))
			!= std::string::npos)
		{
			unit = 1024;
		} else if ((pos = s.find_first_of("Mm"))
			!= std::string::npos)
		{
			unit = 1024 * 1024;
		}
	}
	v = s2n<int>(s) * unit;

	/** set as invalid data */
	if (v == 0 && s.length() == 0) {
		v = -1;
	}
}

void
get_range(const string& value, int& v_min, int& v_max)
{
	int max, min;
	std::string s1 = value;
	std::string s2 = common::string_next(s1);
	fix_unit(min, s1);
	fix_unit(max, s2);

	v_min = std::min(max, min);
	v_max = std::max(max, min);
	if (v_min == -1) {
		v_min = v_max;
	}
}

#define PO_INT32(type)	\
		po::value<int>(&type)->default_value(type)

#define PO_INT64(type)	\
		po::value<int64_t>(&type)->default_value(type)

#define PO_STRI(type)	\
		po::value<string>(&type)->default_value(type)

#define PO_BOOL_SET(type)	\
		po::value<bool>(&type)->default_value(type)

int
main(int argc, char* argv[])
{
	BaseConfig config;
    ObjectConfig object;

	po::options_description opdesc("options");
	opdesc.add_options()
		("help,h", 		"help")

		("clear,c", 	"clear all data")
		("recover,r",	"service recover")
		("sleep",		PO_INT32(config.test.sleep), "sleep before start")
		("wait,w",		PO_INT32(config.test.wait), "wait when complete")
		("seed", 		PO_INT32(config.test.seed), "default 0 for random seed")
		("thread", 		PO_INT32(config.test.thread), "thread count")
		("type",		PO_INT32(config.test.type), "0: put, 1: get")
		("total,t", 	PO_INT64(config.test.total), "total count")
		("batch,b", 	PO_INT32(config.test.batch), ("batch request" + next_unit("data set")).c_str())

		("seqn_key", 	"sequence key")
		("uuid_key", 	"uuid key")
		("uuid_data", 	"uuid data")
		("char_data", 	"use char as data")
		("key_size,k", 	po::value<string>()->default_value("40"), "key size range")
		("data_size,d", po::value<string>()->default_value("4K,8K"), "data size range")

		("sid", 		PO_INT32(object.global.sid), "server id")
		("root", 		PO_STRI(object.global.root), "root directory")
		("wthread", 	PO_INT32(object.writer.thread), "writer thread")
		("unit", 		po::value<string>()->default_value(string_size(object.writer.unit, false)), "unit size")
		("dio",			PO_BOOL_SET(object.writer.io.direct), "use directo io")
		("sync",		PO_BOOL_SET(object.writer.io.sync), "use sync io")

		("host", 		PO_STRI(object.reader.conn.host), "db host")
		("user", 		PO_STRI(object.reader.conn.user), "db user")
		("passwd", 		PO_STRI(object.reader.conn.passwd), "db password")
		("database", 	PO_STRI(object.reader.conn.dbname), "db name");

	po::variables_map vm;
	try {
		po::store(po::parse_command_line(argc, argv, opdesc), vm);
		po::notify(vm);

	} catch (exception& e) {
		cout << "client: " << e.what() << "\n";
		help(opdesc);
	}

	if (vm.count("help")) {
		help(opdesc);
		return -1;
	}

	if (vm.count("clear")) {
		config.test.clear = true;
	}

	if (vm.count("recover")) {
		config.test.recover = true;
	}

	if (vm.count("seqn_key")) {
		config.data.rand_key = false;
	}

	if (vm.count("uuid_key")) {
		config.data.uuid_key = true;
	}

	if (vm.count("uuid_data")) {
		config.data.uuid_data = true;
	}

	if (vm.count("char_data")) {
		config.data.char_data = true;
	}

	if (vm.count("key_size")) {
		get_range(vm["key_size"].as<string>(),
			config.data.key_min,
			config.data.key_max);
	}

	if (vm.count("data_size")) {
		get_range(vm["data_size"].as<string>(),
			config.data.data_min,
			config.data.data_max);
	}

	if (vm.count("unit")) {
		std::string size = vm["unit"].as<string>();
		fix_unit(object.writer.unit, size);
	}

	if (config.test.thread > 1024) {
		cout << "set thread as " << config.test.thread
			 << ", try -t again "<< endl;
		exit(-1);
	}

#if OBJECT_PERFORM
	config.test.batch = 100;
	config.test.total = 1000000;
	config.test.clear = true;
	config.test.thread = 1;
	object.writer.thread = 1;
	object.writer.io.direct = true;
	object.global.root = "/mnt/yfs/object";

#if VIRTUAL_MACHINE
	object.writer.io.direct = false;
	config.test.total = 100000;
#endif
#endif

	GetControl()->SetConfig(&object);

    ClientManage manage;
    manage.Start(config);
    return 0;
}
