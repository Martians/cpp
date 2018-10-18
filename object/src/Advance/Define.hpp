
#pragma once

namespace common {
	/*
	 * example:
	 *
	 * #define ERRNO_MAP(XX)  			\
	 *	XX(DE_exist, "already exist")	\
	 *	XX(DE_drop,  "conn dropped")
	 */
	/** used in enum defination */
	#define ENUM_DEFINE(code, __) 	code,
	/** used in enum map to string */
	#define ENUM_STRING(code, msg)  case code: return msg;
	/** used in enum map to enum name */
	#define NAME_STRING(code, msg)  case code: return #code;
	/** used in initialize */
	#define ENUM_ARRAY(code, msg) 	{code, msg},

	/** define enum type */
	#define DEFINE_ENUM(Type, ENUM_MAP)	\
	enum Type {       					\
		Type ##_null = 0,               \
		ENUM_MAP(ENUM_DEFINE)			\
		Type ##_max,   					\
	}

	/** define enum type */
	#define DEFINE_TYPE_ENUM(Type, ENUM_MAP, TYPE_ENUM_DEFINE)\
	enum Type {       					\
		Type ##_null = 0,               \
		ENUM_MAP(TYPE_ENUM_DEFINE)		\
		Type ##_max,   					\
	}

	/** define func map enum to string */
	#define STRING_ENUM(Name, ENUM_MAP)	\
	const char* Name (int type) { 		\
		switch (type) {                 \
			ENUM_MAP(ENUM_STRING);     	\
		}                               \
		return "";                      \
	}

	/** define func map enum to enum name */
	#define STRING_NAME(Name, ENUM_MAP) \
	const char* Name (int type) {       \
		switch (type) {                 \
			ENUM_MAP(NAME_STRING);      \
		}                               \
		return "";                      \
	}

	/** format initialize array */
	#define INITIAL_MAP(ERRNO_MAP) {    \
		ERRNO_MAP(ENUM_ARRAY)    		\
	}

	/**
	 * #define DataMap(XX)				\
	 *  	XX(DE_conn, "conn")			\
	 *		XX(DE_drop, "drop")
	 *
	 * DEFINE_ENUM(DataError, DataMap);
	 *
     * RegistMap<int, string> s_errmap = INITIAL_MAP(DataMap);
	 **/
}
