
#pragma once

#include <cstring>
#include <random>
#include <memory>

#include "Common/Util.hpp"
#include "Common/Type.hpp"
#include "CodeHelper/Bitset.hpp"

namespace common {
namespace random {

	typedef typeid_t rand_t;

	/**
	 * char table
	 **/
	class CharTable
	{
	public:
		CharTable() { reset(); }

		/**
		 * limit human readable char
		 **/
		enum {
			/** should never lower than max */
			c_char_min = 32,
			/** should never equal max */
			c_char_max = 127,
		};

		/**
		 * limit readable char
		 **/
		struct Range {
			char	min;
			char	max;
		};

	public:
		/**
		 * reset char table
		 **/
		void	reset(bool fill = true);

		/**
		 * regist expected char, shoudl set this only once
		 **/
		void 	expected(std::initializer_list<Range> array);

		/**
		 * regist unexpected char
		 **/
		void 	unexpect(std::initializer_list<char> array);

		/**
		 * get local data
		 **/
		byte_t* data() { return m_data; }

		/**
		 * get current size
		 **/
		size_t	size() { return m_size; }

	public:
		/**
		 * increase char
		 **/
		bool	next_index(byte_t& c);

		/**
		 * convert buffer with data
		 **/
		void	convert(byte_t* data, length_t size) {
			for (byte_t* ptr = data; ptr < data + size; ++ptr) {
				convert(*ptr);
			#if TEST_MODE
				assert(verify(*ptr));
			#endif
			}
		}

		/**
		 * convert ignored char
		 **/
		void	convert(byte_t& c) {
			if ((size_t)c >= m_size) {
				c %= m_size;
			}
			//assert((length_t)c <= sizeof(m_data));
			c = m_data[(size_t)c];
		}
		
		/**
		 * verify current char if in local range
		 **/
		bool	verify(byte_t& c);
		/**
		 * verify current char if in local range
		 **/
		bool	verify(byte_t* data, length_t size);

        /**
		 * get origin index
		 **/
		void	convert_index(byte_t* data, length_t size, bool reverse = false) {
			for (byte_t* ptr = data; ptr < data + size; ++ptr) {
				*ptr = convert_index(*ptr, reverse);
			}
		}

	protected:
		/**
		 * update range
		 **/
		void	update(byte_t c) { m_data[m_size++] = c; }

		/**
		 * get char index
		 **/
		byte_t	convert_index(byte_t v, bool reverse = false);

	protected:
		/** char table for ignore filter */
		byte_t	m_data[c_char_max] = {0};
		size_t	m_size = {0};
	};
	typedef std::shared_ptr<CharTable> table_ptr;

	/**
	 * random generator
	 **/
	class Random
	{
	public:
		typedef std::vector<int64_t> array_t;
		/**
		 * get range weight
		 * @note {[1,2], [2, 3], [5, 6]}
		 **/
		struct PieceRange {
			array_t ranges;
			array_t weight;
		};

		struct Param {
		public:
			Param(rand_t _seed = 0, rand_t _min = 0, rand_t _max = 0,
				int _gen = T_default, int _dist = T_default)
				: seed(_seed), min(_min), max(_max), generator(_gen), distribution(_dist) {}

		public:
			/**
			 * set range and weight
			 **/
			void	range(array_t& ranges, array_t& weight);

			/**
			 * set range and weight
			 **/
			void	range(std::initializer_list<int64_t> ranges, std::initializer_list<int64_t> weight) {
				array_t rv = ranges;
				array_t wv = weight;
				range(rv, wv);
			}

		public:
			/** generator seed */
			rand_t seed = {0};
			/** range min */
			rand_t min = {0};
			/** range max */
			rand_t max = {0};
			/** generator type */
			int	generator = {T_default};
			/** distribution type */
			int	distribution = {T_default};
			/** range weight */
			PieceRange piecewise;
		};

		/**
		 * generator type
		 **/
		enum GeneratorType {
			T_default = 0,

			GT_default = T_default,
			T_minstd,
			T_ranlux,
			T_device,
		};

		/**
		 * distribution type
		 **/
		enum DistributeType {
			DT_default = T_default,
			DT_piecewise_constant,
		};

		enum {
			T_null = 0,
			T_record,
		};
	public:

		Random(Param param);

		virtual ~Random() {
			reset(m_engine);
			reset(m_uniform);
			reset(m_piecewise);
		}

	public:
		/**
		 * set char table
		 **/
		void	char_table(table_ptr table) { m_char = table; }

		/**
		 * get range value
		 **/
		rand_t 	range(length_t max, length_t min = 0) {
			if (max == min) {
				return max;
			}
			return next() % (max - min) + min;
		}

		/**
		 * get next random value
		 **/
		rand_t 	next() { return (*m_uniform)(*m_engine); }

		/**
		 * fill data buffer
		 **/
		void	fill(byte_t* data, size_t size) {
			byte_t* ptr = data;
			byte_t* end = data + size;

			for (; ptr < end; ptr += sizeof(rand_t)) {
				*(rand_t*)ptr = next();
			}
			if (ptr > end) {
				ptr -= sizeof(rand_t);
				fill_min(ptr, end - ptr);
			}
		}

		/**
		 * update date buffer
		 **/
		void	update(byte_t* data, size_t size, length_t count = 0) {
			if (size <= sizeof(rand_t)) {
				fill_min(data, size);

			} else {
				do {
					byte_t* ptr = data + range(size - sizeof(rand_t));
					fill_min(ptr);
				} while (count-- > 0);
			}
		}

        /**
		 * fill minimal buffer
		 **/
		void	fill_min(byte_t* data, size_t size = sizeof(rand_t)) {
			size = std::min(size, (size_t)sizeof(rand_t));
			rand_t value = next();
			memcpy(data, &value, size);

			if (m_char) {
				m_char->convert(data, size);
			}
		}

	public:
		/**
		 * get next piece
		 **/
		rand_t	next_piece() {
			rand_t next = (*m_piecewise)(*m_engine);
			if (get_bit(T_record)) {
				record(next);
			}
			return next;
		}

		/**
		 * record generator
		 **/
		void	record(rand_t number);

		/**
		 * dump piece record result
		 **/
		std::string dump_piece(int64_t max_star = 100);

		BITSET_DEFINE;

	protected:
		/**
		 * local use char or not
		 **/
		bool	use_char() { return m_char != NULL; }

	protected:
		/** random param */
		Param 		m_param;
		/** char table */
		table_ptr 	m_char;
		/** record range distribute */
		array_t		m_record;
		/** generator engine */
		std::default_random_engine* m_engine = {NULL};
		/** uniform distribution */
		std::uniform_int_distribution<int64_t>* m_uniform = {NULL};
		/** piecewise distribution */
		std::piecewise_constant_distribution<float>* m_piecewise = {NULL};
	};
	typedef std::shared_ptr<Random> random_ptr;

	/**
	 * random factory for different type
	 **/
	class RandomFactory
	{
	public:
		/**
		 * set random type
		 **/
		static random_ptr get(Random::Param type);
	};
}
}
using common::random::random_ptr;
