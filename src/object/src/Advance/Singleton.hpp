
#pragma once

#include "Common/Define.hpp"

namespace common {

	/**
	 * Thread-safe, no-manual destroy Singleton template
	 **/
	template<typename Type>
	class Singleton
	{
	public:
		/** get instance */
		static Type* get() {
			pthread_once(&_p_once, &Singleton::_new);
			return _instance;
		}

		static void destory() { _delete(); }

	private:
		Singleton();
		~Singleton();

		/**
		 * Construct the singleton instance
		 */
		static void _new() {
			_instance = new Type();
		}

		/// @brief  Destruct the singleton instance
		/// @note Only work with gcc
		__attribute__((destructor)) static void _delete() {
			typedef char T_must_be_complete[sizeof(Type) == 0 ? -1 : 1];
			(void) sizeof(T_must_be_complete);
			delete _instance;
		}

		static pthread_once_t _p_once;
		static Type* _instance;

	private:
		DISALLOW_COPY_AND_ASSIGN(Singleton);
	};

	template<typename T>
	pthread_once_t Singleton<T>::_p_once = PTHREAD_ONCE_INIT;

	template<typename T>
	T* Singleton<T>::_instance = NULL;
}


