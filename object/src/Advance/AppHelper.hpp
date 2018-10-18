
#pragma once

#include <string>

#include "Common/Define.hpp"
#include "Common/Type.hpp"

namespace common {

	/**
	 * unique id allocator
	 **/
	template<class Type = typeid_t>
	class Unique
	{
	public:
		Unique(Type start = 0) : m_sequence(start) {}

	public:
		/**
		 * get next unique
		 **/
		Type	next() { return ++m_sequence; }

		/**
		 * get current max value
		 **/
		Type	max() { return m_sequence; }

	protected:
		/** unique allocer */
		Type	m_sequence = {0};
	};

	/**
	 * common error parser
	 **/
	class CurrentStatus
	{
	public:
	    CurrentStatus() {}

	public:
		/**
		 * set errno
		 **/
		int 	Errno(int err) {
			m_errno = err;
			return m_errno;
		}

	    /**
	     * get error string
	     **/
	    int 	Errno() { return m_errno; }

	    /**
	     * get error string
	     **/
	    const std::string& Error() { return m_string; }

	    /**
	     * set errno and error string
	     **/
	    int		Error(int error, const std::string& string);

	    /**
	     * set errno and error string
	     **/
	    int		Error(int error, const char* format, ...);

	    /**
	     * reset error info
	     **/
	    void	Reset() { Error(0, 0, ""); }

	public:
		/**
		 * set errno and param
		 *
		 * @param error errno
		 * @param param syserror or other param
		 **/
		int 	Errno(int error, int param) {
			m_errno = error;
			m_param = param;
			return m_param;
		}

	    /**
	     * get error param
	     **/
	    int		Param() { return m_param; }

	    /**
	     * set errno and error string
	     **/
	    int		Error(int error, int param, const std::string& string) {
	    	m_param = param;
	    	return Error(error, string);
	    }

	public:
	    /** errno number */
	    int		m_errno	= {0};
	    /** error param */
	    int		m_param	= {0};
	    /** error string */
	    std::string	m_string;
	};
}

