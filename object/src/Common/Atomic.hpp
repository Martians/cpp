
#pragma once

namespace common {

	/**
	 * @return old value of mem
	 **/
	static inline int
	atomic_add(volatile int *mem, int add) {
		asm volatile(
				"lock xadd %0, (%1);"
				: "=a"(add)
				: "r"(mem), "a"(add)
				: "memory"
		);
		return add;
	}

	static inline int64_t
	atomic_add64(volatile int64_t* mem, int64_t add) {
		asm volatile(
				"lock xaddq %0, (%1)"
				: "=a" (add)
				: "r" (mem), "a" (add)
				: "memory"
		);
		return add;
	}

	static inline void
	atomic_inc(volatile int *mem) {
		asm volatile(
				"lock incl %0;"
				: "=m"(*mem)
				: "m"(*mem)
		);
	}

	static inline void
	atomic_inc64(volatile int64_t *mem) {
		asm volatile(
				"lock incq %0;"
				: "=m"(*mem)
				: "m"(*mem)
		);
	}

	static inline void atomic_dec(volatile int *mem) {
		asm volatile(
				"lock decl %0;"
				: "=m"(*mem)
				: "m"(*mem)
		);
	}

	static inline void
	atomic_dec64(volatile int64_t *mem) {
		asm volatile(
				"lock decq %0;"
				: "=m"(*mem)
				: "m"(*mem)
		);
	}

	/**
	 * @return old value
	 * @note only the first one will get the old one which not equal value
	 * @note others will get return of the value param
	 **/
	static inline int
	atomic_swap(volatile void *lockword, int value) {
		asm volatile(
				"lock xchg %0, (%1);"
				: "=a"(value)
				: "r"(lockword), "a"(value)
				: "memory"
		);
		return value;
	}

	static inline int64_t
	atomic_swap64(volatile void *lockword, int64_t value) {
		asm volatile(
				"lock xchg %0, (%1);"
				: "=a"(value)
				: "r"(lockword), "a"(value)
				: "memory"
		);
		return value;
	}

	/**
	 * @note if success, return the old mem value, equal cmp
	 * @note if failed, return the value of mem
	 **/
	static inline int
	atomic_comp_swap(volatile void *mem, int xchg, int cmp) {
		asm volatile(
				"lock cmpxchg %1, (%2)"
				:"=a"(cmp)
				:"d"(xchg), "r"(mem), "a"(cmp)
		);
		return cmp;
	}

	static inline int64_t
	atomic_comp_swap64(volatile void *mem, int64_t xchg, int64_t cmp) {
		asm volatile(
				"lock cmpxchg %1, (%2)"
				:"=a"(cmp)
				:"d"(xchg), "r"(mem), "a"(cmp)
		);
		return cmp;
	}

	/**
	 * reference counter
	 **/
	class Refer
	{
	public:
		/**
		 * increase refernce
		 **/
		void	inc() { atomic_inc(&refer); }

		/**
		 * decrease reference
		 **/
		bool	dec() { return atomic_add(&refer, -1) == 1; }

		/**
		 * get reference
		 **/
		int		ref() { return refer; }

	public:
		int 	refer = {1};
	};

	/** fast macro */
	#define at_inc(x) 	common::atomic_add(&x, 1)
	#define at_dec(x) 	common::atomic_add(&x, -1)
	#define at_inc64(x) common::atomic_add64(&x, 1)
	#define at_dec64(x) common::atomic_add64(&x, -1)
} // namespace common

