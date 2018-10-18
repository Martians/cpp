
#pragma once

#include <cstdlib>

namespace common {

	/**
	 * reset pointer
	 **/
	template<class T>
	void reset(T& ptr) {
		if (ptr) {
			delete ptr;
			ptr = NULL;
		}
	}

	/**
	 * reset pointer
	 **/
	template<class T>
	void reset_array(T& ptr) {
		if (ptr) {
			delete[] ptr;
			ptr = NULL;
		}
	}

	/**
	 * set bitmap
	 **/
	template<class T, class V>
	void set_bit(T& state, V bit, bool set = true) {
		T mask = 1 << (bit - 1);
		if (set) {
			state |= mask;
		} else {
			state &= ~mask;
		}
	}

	/**
	 * get bit map
	 **/
	template<class T, class V>
	bool get_bit(T state, V bit) {
		return state & ((T)1 << (bit - 1));
	}

	/**
	 * format bit map
	 **/
	template<class T = int, class V>
	T 	make_bit(V bit) {
		if (bit == 0) {
			return 0;
		}
		return (T)1 << (bit - 1);
	}

	/**
	 * format bit map
	 **/
	template<class T = int, class V, class ... Types>
	T 	make_bit(V bit, Types...args) {
		return make_bit(bit) | make_bit(args...);
	}
}

#if COMMON_SPACE
	using common::reset;
	using common::make_bit;
#endif
