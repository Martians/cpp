
#pragma once

#include <string>

#include "Common/Const.hpp"
#include "Common/Util.hpp"
#include "Common/CodeHelper.hpp"
#include "Common/DataType.hpp"
#include "Perform/Random.hpp"

namespace common {
namespace random {

	/**
	 * limit human readable char
	 **/
	enum {
		/** source local table size */
		c_local_rand_size = c_length_64K * 4,
		/** default range size */
		c_invalid_size = -1,
	};

	/**
	 * update rand seed
	 **/
	rand_t	update_seed(rand_t& seed);

	/**
	 * raw data generator table
	 **/
	class SourceTable
	{
	public:
		enum {
			/** table unit count */
			unit_count = 256,
			/** table unit size */
			unit_size  = c_length_1K,
		};

		/**
		 * default operation unit
		 **/
		struct Unit
		{
		public:
			/**
			 * initial raw table unit
			 **/
			void 	initial(random_ptr rand);

			/**
			 * update with char table
			 **/
			void	convert(table_ptr table);

			/**
			 * verify char table
			 **/
			void	verify(table_ptr table);

		public:
			/** raw unit data */
			byte_t  data[unit_size] = {0};
		};

	public:
		/**
		 * initial with seed
		 **/
		void		initial(rand_t seed = 0, int rand_type = 0, bool use_char = false);

		/**
		 * regist expected char, should set this only once
		 **/
		void 		expected(std::initializer_list<CharTable::Range> array);

		/**
		 * regist unexpected char
		 **/
		void 		unexpect(std::initializer_list<char> array);

		/**
		 * get char table
		 **/
		table_ptr	char_table() { return m_char; }

		/**
		 * get data start
		 **/
		const byte_t* data(random_ptr rand) {
			length_t off = (length_t)rand->range(ending());
			return (byte_t*)m_array + off;
		}

		/**
		 * get generator size
		 **/
		length_t	size() { return unit_size;}

		/**
		 * get table ending, last unit start
		 **/
		length_t	ending() {
			static length_t s_ending = (unit_count - 1) * unit_size - unit_size;
			return s_ending;
		}

		/**
		 * check if table char valid
		 **/
		void		char_verify();

	protected:
		/**
		 * create char table
		 **/
		void		create_table();

		/**
		 * convert table when update char table
		 **/
		void		char_convert();

	public:
		/** unit array */
		Unit	m_array[unit_count];
		/** char table */
		table_ptr m_char;
		/** initialize random */
		random_ptr m_rand;
	};
	SINGLETON(SourceTable, source_table);
	SINGLETON(SourceTable, chared_table);

	/**
	 * base generator
	 **/
	class Source
	{
	public:
		Source() {}
		virtual ~Source() { cycle(); }

		/**
		 * source construct param
		 **/
		struct Param
		{
		public:
			Param(int type, length_t size, length_t max_size = c_invalid_size, bool use_char = false);

		public:
			/**
			 * source base info
			 **/
			struct Base {
				/** source type */
				int		type = {T_rand};
				/** use char or not */
				bool	use_char = {false};
				/** data buffer size */
				length_t size = {0};
			} base;

			/**
			 * random table info
			 **/
			struct Table {
				/** table random seed */
				int64_t	seed = {100};
				/** random type */
				int		rand = {Random::T_default};
			} table;

			/**
			 * sequence source
			 **/
			struct Seqn {
				/** seqn source start data */
				byte_t* start = {NULL};
			} seqn;

			/**
			 * random source
			 **/
			struct Rand {
				/** rand source max size */
				length_t max_size = {0};
			} rand;
		};

		/**
		 * source type
		 **/
		enum Type {
			T_rand = 0,
			T_seqn,
			T_uuid,
		};

		/**
		 * get source enum type
		 **/
		static int	parse_type(bool rand, bool uuid = false);

	public:
		/**
		 * initial local data pool
		 **/
		virtual void initial(Source::Param source, Random::Param random);

		/**
		 * generator data range
		 *
		 * @output data buffer start
		 * @output size data buffer len
		 * @param specific length for single use
		 **/
		virtual void get(Data* data, length_t specific = 0) = 0;

		/**
		 * get current data
		 **/
		byte_t* data() { return m_data; }

		/**
		 * get generator data size
		 **/
		length_t size() { return m_size; }

		/**
		 * get char table
		 **/
		void	char_table(table_ptr table) { m_char = table; }

		/**
		 * get char table
		 **/
		table_ptr char_table() { return m_char; }

	protected:
		/**
		 * update table when char table change
		 **/
		virtual void char_update();

		/**
		 * extend buffer size
		 **/
		void	extend(length_t size);

		/**
		 * cycle data if new malloced
		 **/
		void	cycle() { reset_array(m_data); }

		/**
		 * local use char or not
		 **/
		bool	use_char() { return m_char != NULL; }

	protected:
		/** local generator data pool */
		byte_t*  m_data = {NULL};
		/** data length */
		length_t m_size = {0};
		/** char table */
		table_ptr m_char;
		/** local random */
		random_ptr m_rand;
	};

	/**
	 * sequence generator
	 **/
	class SeqnSource : public Source {

	public:
		using Source::Source;
		virtual ~SeqnSource();

	public:
		/**
		 * initialize rand
		 **/
		virtual void initial(Source::Param source, Random::Param random);

		/**
		 * generator data
		 **/
		virtual void get(Data* data, length_t specific = 0) {
			next();

			data->ptr = m_data;
			data->len = m_size;
		}

		/**
		 * increase current sequence
		 **/
		void	increase(rand_t step);

	protected:
		/**
		 * do update when char table change
		 *
		 * @note keep swap same as data buffer
		 **/
		virtual void char_update();

		/**
		 * swap to data buffer, convert table index to char
		 *
		 * @note keep data buffer newest
		 **/
		void	swap_data();

		/**
		 * get next sequence
		 **/
		void 	next();

		/**
		 * get table size
		 **/
		size_t	scale() { return m_char ? m_char->size() : (1 << 8); }

		/**
		 * get beg char pointer
		 **/
		byte_t* beg() { return m_swap ? m_swap : m_data; }

		/**
		 * get end char pointer
		 **/
		byte_t*	end() { return m_swap ? m_swap + m_size - 1 : m_data + m_size - 1; }

		/**
		 * convert step to index scale format
		 **/
		void 	scale_step(rand_t step, byte_t* ptr);

		/**
		 * increase data by step
		 **/
		void	increase(byte_t* temp);

	protected:
		/** data swap */
		byte_t*	 m_swap = {NULL};
	};

	/**
	 * random generator, always use local_size as local data buffer
	 **/
	class RandSource : public Source {
	public:
		using Source::Source;

		/**
		 * set rand range
		 **/
		length_t	range(Source::Param source);

		/**
		 * initialize rand
		 **/
		virtual void initial(Source::Param source, Random::Param random);

		/**
		 * generate data
		 **/
		virtual void get(Data* data, length_t specific = 0);

	protected:
		/**
		 * char table update
		 **/
		virtual void char_update();

		/**
		 * set last byte as 0, then revert. in case set too many 0
		 **/
		void		keep(byte_t* data = NULL);

	protected:
		/** min length */
		length_t m_min = {0};
		/** max length */
		length_t m_max = {0};
		/** keep last position */
		char*	m_post = {NULL};
		/** keep last char value */
		char	m_last = {0};
		/** local random */
		random_ptr m_local;
	};

	/**
	 * uuid generator
	 **/
	class UUIDSource : public Source {
	public:
		UUIDSource() {}

	public:
		/**
		 * initialize rand
		 **/
		virtual void initial(Source::Param source, Random::Param random);

		/**
		 * get uuid data
		 **/
		virtual void get(Data* data, length_t specific = 0);
	};

	typedef std::shared_ptr<Source> source_ptr;
	/**
	 * source generator
	 **/
	class SourceFactory
	{
	public:
		/**
		 * get source instance
		 **/
		static source_ptr get(Source::Param source,
			Random::Param random = Random::Param());
	};
}
}
using common::random::source_ptr;

