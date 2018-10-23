
#pragma once

namespace common {
	/** used for global check for the class */
	#define	VALIDATE_DEFINE			\
		static void	validate();		\
		static int s_validate;

	#define	VALIDATE(Type)			\
		int Type::s_validate = {	\
			[](){ Type::validate();	\
			return 0; }()			\
		};							\
		void Type::validate()
}
