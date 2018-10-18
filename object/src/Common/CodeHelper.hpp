
#pragma once

namespace common {

	/**
	 * singleton code generate machine
	 **/
	#define SINGLETON(Type, name)		\
		inline Type& name() {			\
		static Type s_local;       		\
		return s_local;		    		\
	}

	/**
	 * A macro to disallow the copy constructor and operator= functions
	 * This should be used in the priavte:declarations for a class
	 */
	#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
			TypeName(const TypeName&);  TypeName& operator=(const TypeName&);
}

