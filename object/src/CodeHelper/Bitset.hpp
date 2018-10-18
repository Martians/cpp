
#pragma once

#include "Common/Util.hpp"

namespace common {
	/**
	 * define bit set in class
	 **/
	#define	BITSET_DEFINE				\
		bool get_bit(int bit) const {	\
			return common::get_bit(m_bits, bit);\
		}								\
		void set_bit(int bit, bool set = 1) {	\
			common::set_bit(m_bits, bit, set);	\
		}								\
		bool reset_bit(int bit = 0) {	\
			bool curr = get_bit(bit);	\
			if (bit == 0) {				\
			 	m_bits = 0;				\
			} else if (curr) {			\
				set_bit(bit, false);    \
			}							\
			return curr;				\
		}								\
		bool change(int bit, bool set) {\
			if (get_bit(bit) == set) {	\
				return false;			\
			}							\
            set_bit(bit, set);	        \
			return true;				\
		}								\
		int	m_bits = {0};
}

