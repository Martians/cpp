
#include "Applet/Client/ConfigBase.hpp"
#include "Applet/Client/Command.hpp"

using namespace applet::client;

int
main(int argc, char* argv[])
{
	class ClientConfig : public Config {
	public:
		struct Extend {
			std::string	path;
			int 		time = 0;
			bool		check = false;
		} extend;
	};
	ClientConfig config;

	po::options_description opdesc("extend");
	opdesc.add_options()
		("path,p", 	PO_TYPE(config.extend.path), "config path")
		("time",	PO_TYPE(config.extend.time), "wait time")
		("check",	PO_BOOL(config.extend.check), "check flag");

	Command command(&config);
	command.add_option(opdesc);
	command.parse(argc, argv);

	std::cout << config.dump("base:\t") << std::endl
		<< "\nextend options:\n"
		<< "path: \t" << config.extend.path << std::endl
		<< "time: \t" << config.extend.time << std::endl
		<< "check:\t"  << config.extend.check << std::endl;

	config.dump("test suit, ");
	return 0;
}



