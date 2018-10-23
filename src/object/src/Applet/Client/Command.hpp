
#pragma once

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options;

namespace applet {
namespace client {

	/** define option receiver */
	#define PO_TYPE(type)			po::value<decltype(type)>(&type)->default_value(type)
	/** define option receiver */
	#define PO_BOOL(type)			po::bool_switch(&type)
	/** define option receiver */
	#define PO_TYPE_EMPTY(type)		po::value<decltype(type)>(&type)

	/** automate parse option value, which has the same name with its member name */
	#define PO_REV_BOOL(command, refer, name)	(command).set_bool(refer, name, true)
	/** automate parse option value, which has the same name with its member name */
	#define PO_SET_VAL(command, refer)	(command).set(refer, #refer)

	class Config;

	/**
	 * command base
	 *
	 * we can use command line, command file, or ini configure file for work
	 **/
	class Command
	{
	public:
		Command(Config* config) : m_config(config) {}

		/**
		 * add more options
		 **/
		void	add_option(po::options_description& opdesc) { m_option.add(opdesc); }

		/**
		 * parse argument
		 **/
		void	parse(int argc, char* argv[]);

	public:
		/**
		 * set current type
		 **/
		template <class Type>
		bool	set(Type& value, const char* name){
			name = validate_name(name);
			if (m_varmap.count(name)) {
				value = m_varmap[name].as<Type>();
				return true;
			}
			return false;
		}

		/**
		 * set bool type
		 **/
		bool	set_bool(bool& value, const char* name, bool revert = false){
			name = validate_name(name);
			if (m_varmap.count(name)) {
				value = revert ? false : true;
				return true;
			}
			return false;
		}

		/**
		 * check if two option is conflict
		 **/
		void	conflict(const std::string& opt1, const std::string& opt2);

		/**
		 * check if option dependent is satisfied
		 **/
		void	depend(const std::string& for_what, const std::string& required_option);

		/**
		 * get var map for extend parse
		 **/
		po::variables_map&	var_map() { return m_varmap; }

	protected:
		typedef po::positional_options_description position;
		typedef po::options_description options;
		typedef po::variables_map varmap;

		/**
		 * add default config set
		 **/
		void	default_options();

		/**
		 * parse command line
		 **/
		void	parse_command(int argc, char* argv[]);

		/**
		 * parse configure
		 **/
		void	parse_configure();

		/**
		 * check default conflict
		 **/
		void	check_conflict();

		/**
		 * parse default config set
		 **/
		void	default_parse();

		/**
		 * parse option name by total option
		 **/
		const char* validate_name(const char* name);

		/**
		 * output help info
		 **/
		void	help();

        const uint32_t c_commandl = 90;
        const uint32_t c_descripl = 30;

	public:
//    	options	m_help = {"help", c_commandl, c_descripl};
//		options	m_host = {"host", c_commandl, c_descripl};
//		options	m_load = {"load", c_commandl, c_descripl};
//		options	m_work = {"work", c_commandl, c_descripl};
//		options	m_data = {"data", c_commandl, c_descripl};

        Config* m_config = {NULL};
		options m_option = {"options", c_commandl, c_descripl};
		varmap 	m_varmap;
		position m_position;
	};
}
}
