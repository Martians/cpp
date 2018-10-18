
#pragma once

#include <cstdlib>

namespace common {

	/**
	 * @brief up mode by tick
	 * @param value the src value
	 * @param tick mode tick
	 * @return result
	 * @note tick must be 2-degree power, then it is faster
	 * than other func
	 */
	template<class Type, class Tick>
	Type up_align(Type value, Tick tick) {
		Type align = (Type)tick;

		if (value == 0) {
			return align;
		} else {
			return (value + align - 1) & ~(align - 1);
		}
	}

	/**
	 * @brief down mode by tick
	 * @param value the src value
	 * @param tick mode tick
	 * @return result
	 * @note tick must be 2-degree power, then it is faster
	 * than other func
	 */
	template<class Type, class Tick>
	Type dw_align(Type value, Tick tick) {
		Type align = (Type)tick;
		return value & ~(align - 1);
	}

	/**
	 * @brief mode tick
	 * @note tick must be 2-degree power, then it is faster
	 */
	template<class Type, class Tick>
	bool aligned(Type value, Tick tick) {
		Type align = (Type)tick;
		return (value & (align - 1)) == 0;
	}

	/**
	 * @brief up mode by tick
	 * @param value the src value
	 * @param tick mode tick
	 * @return result
	 * @note tick could not be 2-degree power
	 */
	template<class Type, class Tick>
	Type up_round(Type value, Tick tick) {
		Type align = (Type)tick;

		if (value == 0) {
			return align;
		} else {
			return (value + align - 1) / align * align;
		}
	}

	/**
	 * @brief down mode by tick
	 * @param value the src value
	 * @param tick mode tick
	 * @return result
	 * @note tick could not be 2-degree power
	 */
	template<class Type, class Tick>
	Type dw_round(Type value, Tick tick) {
		Type align = (Type)tick;
		return value / align * align;
	}

	/**
	 * @brief up exponent with value
	 */
	template<class Type>
	int up_exp(Type value) {
		int exp = 0; Type seed = 1;
		while (1) {
			if (seed >= value) break;
			seed <<= 1; exp++;
		}
		return exp;
	}

	/**
	 * @brief down exponent with value
	 */
	template<class Type>
	int dw_exp(Type value) {
		int exp = 0; Type seed = 1;
		while (1) {
			if (seed >= value) break;
			seed <<= 1; exp++;
		}
		return seed == value ? exp : exp - 1;
	}

	/**
	 * test if value is 2-exp based
	 **/
	template<class Type>
	bool exp_base(Type v) {
        return (((Type)1 << up_exp(v)) == v);
    }
}
