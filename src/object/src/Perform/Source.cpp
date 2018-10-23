
#include <cstring>
#include <unistd.h>
#include <algorithm>

#include "Common/Define.hpp"
#include "Common/Display.hpp"
#include "Common/Mutex.hpp"
#include "Common/Reference.hpp"
#include "CodeHelper/Trigger.hpp"
#include "Advance/Util.hpp"
#include "Perform/Source.hpp"

namespace common {
namespace random {

rand_t
update_seed(rand_t& seed)
{
	if (seed == 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		seed = tv.tv_usec;
	}
	return seed;
}

void
SourceTable::initial(rand_t seed, int type, bool use_char)
{
	static Mutex s_mutex("");
	Mutex::Locker lock(s_mutex);

	if (m_rand) {
		return;
	}
	update_seed(seed);

    Random::Param param(seed, 0, 0, type, Random::T_default);
    m_rand = RandomFactory::get(param);

	for (size_t i = 0; i < unit_count; i++) {
		m_array[i].initial(m_rand);
	}

	/** char_table maybe already exist */
	if (use_char) {
		create_table();
		char_convert();
	}
}

void
SourceTable::expected(std::initializer_list<CharTable::Range> array)
{
	create_table();

	m_char->expected(array);
	char_convert();
}

void
SourceTable::unexpect(std::initializer_list<char> array)
{
	create_table();

	m_char->unexpect(array);
	char_convert();
}

void
SourceTable::create_table()
{
	if (!m_char) {
		m_char = std::make_shared<CharTable>();
	}
}

void
SourceTable::char_convert()
{
	/** only check when initialized */
	if (m_rand) {
		for (size_t i = 0; i < unit_count; i++) {
			m_array[i].convert(m_char);
		}

		char_verify();
	}
}

void
SourceTable::char_verify()
{
	#if TEST_MODE
	for (size_t i = 0; i < unit_count; i++) {
		m_array[i].verify(m_char);
	}
	#endif
}

void
SourceTable::Unit::initial(random_ptr rand)
{
	rand->fill(data, sizeof(data));
}

void
SourceTable::Unit::convert(table_ptr table)
{
	if (table) {
		table->convert(data, sizeof(data));
	}
}

void
SourceTable::Unit::verify(table_ptr table)
{
	if (table) {
		table->verify(data, sizeof(data));
	}
}

source_ptr
SourceFactory::get(Source::Param source,
		Random::Param random)
{
	/** only active for the first set */
	source_table().initial(source.table.seed, source.table.rand);
	chared_table().initial(source.table.seed, source.table.rand, true);

	source_ptr src;
	switch (source.base.type) {
	case Source::T_seqn: {
		src = std::make_shared<SeqnSource>();
	} break;

	case Source::T_rand: {
        src = std::make_shared<RandSource>();
	} break;

	case Source::T_uuid:
		src = std::make_shared<UUIDSource>();
		break;
	default:
		assert(0);
		break;
	}
	src->initial(source, random);
	return src;
}

Source::Param::Param(int type, length_t size, length_t max_size, bool use_char)
{
	base.type = type;
	base.size = size;
	base.use_char = use_char;
   	rand.max_size = max_size;
}

int
Source::parse_type(bool rand, bool uuid)
{
	if (uuid) {
		return Source::T_uuid;

	} else if (rand) {
		return Source::T_rand;

	} else {
		return Source::T_seqn;
	}
}

void
Source::initial(Source::Param source, Random::Param random)
{
	m_rand = RandomFactory::get(random);
	extend(source.base.size);

	if (source.base.use_char) {
		m_char = chared_table().char_table();
		m_rand->char_table(m_char);

		char_update();
	}
}

void
Source::char_update()
{
	if (m_char) {
		m_char->convert(m_data, m_size);
	}
}

void
Source::extend(length_t size)
{
	cycle();

	if (size != 0) {
		m_size = size;

		size = common::up_align(size + 1, 8);
        assert(m_size < size);
		m_data = new byte_t[size];
		memset(m_data, 0, size);

		SourceTable& table = source_table();
		for (length_t off = 0, len = 0; off < m_size; off += len) {
			len = std::min(size - off, table.size());
			memcpy(m_data + off, table.data(m_rand), len);
		}
	}
}

SeqnSource::~SeqnSource()
{
	reset_array(m_swap);
}

void
SeqnSource::initial(Source::Param source, Random::Param random)
{
	Source::initial(source, random);

	if (source.seqn.start) {
		memset(m_data, 0, m_size + 1);
		length_t size = std::min(m_size, (length_t)strlen(source.seqn.start));
		memcpy(m_data, source.seqn.start, size);
		char_update();

	} else {
		/** update initial sequence buffer */
		m_rand->update(m_data, m_size, 4);
		m_data[m_size] = 0;
	}
	/** data end must have a \0 */
	assert(m_data[m_size] == 0);
}

void
SeqnSource::char_update()
{
	Source::char_update();

	/** only need swap when use char table */
	if (m_char) {
		if (!m_swap) {
			m_swap = new char[m_size + 1];
		}
		memcpy(m_swap, m_data, m_size + 1);
		m_char->convert_index(m_swap, m_size);
	}
}

void
SeqnSource::increase(rand_t step)
{
	byte_t* temp = new byte_t[common::up_align(m_size + 1, 8)];
	memset(temp, 0, m_size);
	scale_step(step, temp);
	increase(temp);
	delete[] temp;
}

void
SeqnSource::scale_step(rand_t step, byte_t* ptr)
{
	while (step > 0) {
		*ptr = step % scale();
		step /= scale();
		ptr++;
	}
}

void
SeqnSource::swap_data()
{
	if (m_char) {
		assert(m_swap != NULL);
		memcpy(m_data, m_swap, m_size);
		m_char->convert_index(m_data, m_size, true);
	}
}

void
SeqnSource::increase(byte_t* temp)
{
	byte_t* ptr = temp;
	size_t carry = 0;

	if (m_char) {
		assert(m_swap != NULL);
	}
	for (byte_t* data = end(); data >= beg(); --data, ++ptr) {
		size_t value = (size_t)((u_byte_t)*data + (u_byte_t)*ptr) + carry;
		if (value < scale()) {
			*data = (u_byte_t)value;
			carry = 0;
			/** in case flag extend with 1 */
			assert((size_t)(u_byte_t)*data < scale());

		} else {
			assert(value - scale() < scale());
			*data = (byte_t)(value - scale());
			carry = 1;
		}
	}

	if (carry) {
		/** no need carry if overflow */
		#if 0
		memset(temp, m_size, 0);
		temp[0] = 1;
		increase(temp);
		#endif
	}
	swap_data();
}

void
SeqnSource::next()
{
	byte_t* ptr = end();

	while (1) {
		++(*ptr);

		if (use_char()) {
			if (m_char->next_index(*ptr)) {
				break;
			}
		} else {
			if ((*ptr) != 0) {
				break;
			}
		}

		if (ptr == beg()) {
			/** move to the highest, all bit will be 0, it is a valid value */
			/** no need carry if overflow */	
			break;

		} else {
			--ptr;
		}
	}
	swap_data();
}

void
RandSource::initial(Source::Param source, Random::Param random)
{
	source.base.size = range(source);
	m_local = RandomFactory::get(Random::Param(random.seed + 100));

	Source::initial(source, random);
}

void
RandSource::char_update()
{
	Source::char_update();

	if (m_char) {
		assert(m_local != NULL);
		m_local->char_table(m_char);
	}
}

length_t
RandSource::range(Source::Param source)
{
	m_min = std::min(source.rand.max_size, source.base.size);
	m_max = std::max(source.rand.max_size, source.base.size);
	{
		if (m_min == c_invalid_size) {
			m_min = m_max;
			assert(m_min != c_invalid_size);
		} else {
			m_min = std::max(1, m_min);
		}
	}
	return c_local_rand_size;
}

void
RandSource::get(Data* data, length_t specific)
{
	SourceTable& table = m_char ? chared_table() : source_table();
	/** get data random size */
	length_t size = (specific == 0) ? m_local->range(m_max, m_min) : specific;
	//chared_table().char_verify();

	keep();

	if (size <= (length_t)sizeof(rand_t)) {
		data->ptr = m_data;

	} else {
		/** random copy raw table data, just do one copy */
		static length_t s_ending = Source::size() - table.size();
		length_t offset = 0;

		/** using s_end or not is ok, we just reserve some data for later copy */
		offset = m_local->range(s_ending - size);
		data->ptr = m_data + offset;

		/** copy head data form global */
		memcpy(m_data + offset, table.data(m_local),
			size < table.size() ? size : table.size());

		int count = use_char() ? 4 : 2;
		/** set new data random */
		m_local->update(data->ptr, size, count);

		#if TEST_MODE
		if (m_char) {
			success(m_char->verify(data->ptr, size));
		}
		#endif
	}
	data->len = size;
	/** fill random head match distribution */
	m_rand->fill_min(data->ptr, data->len);

	keep(data->ptr + data->len);
}

void
RandSource::keep(byte_t* data)
{
	if (data) {
		/** keep last char */
		m_post = data;
		m_last = *m_post;
		/** set last char as 0 */
		*m_post = 0;

	} else if (m_post) {
		*m_post = m_last;
	}
}

void
UUIDSource::initial(Source::Param source, Random::Param random)
{
	source.base.size = 32 + 1;

	Source::initial(source, random);
}

void
UUIDSource::get(Data* data, int len)
{
#if USING_UUID
	get_uuid(m_data);
#else
	assert(0);
#endif
	data->ptr = m_data;
	data->len = m_size;
}

}
}

#if COMMON_TEST

#include "Common/Container.hpp"
#include "Common/Display.hpp"
#include "Common/Time.hpp"
#include "Common/Mutex.hpp"
#include "Common/Logger.hpp"
#include "Perform/Debug.hpp"

namespace common {
namespace tester {

	using namespace common::random;
	using namespace common::helper;

	static int min_loop = 50000;
	static int max_loop = 500000;
	static int overflow = 70000;

	static char s_temp[c_length_64K] = {0};
	static TypeSet<std::string> s_conflict;
	static int s_count = 0, s_total = 0;
	static int s_check = 0;

	auto increase_check = [&](Data& data) {
		if (trigger_check(s_check, false)) {
			if (memcmp(s_temp, data.ptr, data.len) > 0) {
				assert(0);
			}
		}
		memcpy(s_temp, data.ptr, data.len);
	};
	auto overflow_check = [&](Data& data) {
		if (trigger_check(s_check, false)) {
			if (memcmp(s_temp, data.ptr, data.len) > 0) {
				log_info("over flow, last: " << data_trace(s_temp, 2) << ", curr: " << data_trace(data.ptr, 2));
			}
		}
		memcpy(s_temp, data.ptr, data.len);
	};

	auto unexpect_source = [&]() {
		chared_table().char_table().reset();
		chared_table().unexpect({'\\', 'A', '%'});
	};
	auto expected_source = [&]() {
		chared_table().char_table().reset();
		chared_table().expected({{'A', 'Z'}, {'b', 'h'}});
		chared_table().unexpect({'\\', 'A', '%'});
	};

	auto conflict_check = [&](Data& data) {
		std::string v(data.ptr, data.len);
		if (s_conflict.get(&v)) {
			s_count++;
		} else {
			s_conflict.add(&v);
			s_total++;
		}
	};
	auto conflict_clear = [&]() {
		s_conflict.clear();
		s_count = 0;
		s_total = 0;
	};
	auto output_handle = [&](Data& data) {
		log_info("conflict count " << s_count << ", exist " << s_total);
	};
	auto last_handle = [&](Data& data) {
		static Mutex s_mutex("");
		Mutex::Locker lock(s_mutex);
		static std::string s(data.ptr, data.len);
		if (memcmp(s.c_str(), data.ptr, s.length()) != 0) {
			assert(0);
		}
	};

	struct RandHandle {
		typedef std::function<void(Data& data)> handle_t;
		typedef std::function<void(source_ptr src)> source_t;
		handle_t data;
		//source_t source;
		handle_t output;
		handle_t last;
	};

	void
	__source_test(Source::Param param, int loop, RandHandle handle) {
		auto src = SourceFactory::get(param);
//		if (handle.source) {
//			handle.source(src);
//		}
		TimeRecord time;
		Data data;

		int output = loop / 10;
		for (int index = 0; index < loop; index++) {
			src->get(&data);

			if (handle.data) {
				handle.data(data);
			}
			if (index % output == 0) {
				if (handle.output) {
					handle.output(data);
				} else {
					log_info("<" << index << "\t [" << loop << "]: len : " << data.len << ", data : " << std::string(data.ptr, data.len));
				}
			}
		}
		if (handle.output) {
			handle.output(data);
		}
		if (handle.last) {
			handle.last(data);
		}
		time.check();
		log_info("using " << string_elapse(time.last()));
	}

	void
	seqn_source_test() {

		/** 1) check seqn loop, should just be monotonous rising */
		{
			s_check = 0;
			RandHandle handle = {increase_check};
			__source_test(Source::Param(Source::T_seqn, 20, 20), min_loop, handle);
		}

		/** 2) check seqn loop, should overflow a cycle */
		{
			s_check = 0;
			RandHandle handle = {overflow_check};
			__source_test(Source::Param(Source::T_seqn, 2, 2), overflow, handle);
		}

		/** 3) check unexpect char and expect char */
		{
			unexpect_source();

			s_check = 0;
			RandHandle handle = {increase_check};
			__source_test(Source::Param(Source::T_seqn, 4, 4, true), min_loop, handle);
		}

		/** 4) check expect char */
		{
			expected_source();

			s_check = 0;
			RandHandle handle = {overflow_check};
			__source_test(Source::Param(Source::T_seqn, 2, 2, true), overflow, handle);
		}

		/** 5) seqn increase */
		{
			char data[] = {"123456A"};
			auto check_seqn = [&](size_t size, int64_t count, bool use_char = true, bool expect = true) {
				Source::Param param(Source::T_seqn, size);
				param.base.use_char = use_char;
				param.seqn.start = data;

				chared_table().char_table().reset();
				if (use_char && expect) {
					chared_table().expected({{'0', '9'}, {'A', 'Z'}});
				}

				auto next_seqn = [&]() {
					auto seqn = std::dynamic_pointer_cast<SeqnSource>(SourceFactory::get(param));
					return seqn;
				};
				auto seqn = next_seqn();
				auto save = next_seqn();

				seqn->increase(count);
				while (--count >= 0) {
					Data data;
					save->get(&data);
				}
				log_info(seqn->data() << " - " << save->data());
				assert(memcmp(seqn->data(), save->data(), save->size()) == 0);
			};
			check_seqn(3, 1000);
			check_seqn(3, 1000000);
			check_seqn(strlen(data), 120000);

			//for (int i = 0; i < 1000; i++) {
			//check_seqn(3, i, false);
			//}
			check_seqn(3, 1000, false);
			check_seqn(3, 1000000, false);
			check_seqn(3, 1000, true, false);
			check_seqn(3, 1000000, true, false);
		}
	}

	void
	rand_source_test() {

		/** multi thread test, all should have same output */
		{
			expected_source();
			RandHandle handle = {NULL, NULL, last_handle};
			batchs(10, __source_test, Source::Param(Source::T_rand, 20, 20, true), min_loop, handle);
			thread_wait();
		}

		{
			unexpect_source();

			conflict_clear();
			RandHandle handle = {conflict_check, output_handle};
			__source_test(Source::Param(Source::T_rand, 20, 20, true), max_loop, handle);
		}

		{
			conflict_clear();
			RandHandle handle = {conflict_check, output_handle};
			__source_test(Source::Param(Source::T_rand, 4, 4), max_loop, handle);
		}

		{
			conflict_clear();
			RandHandle handle = {conflict_check, output_handle};
			__source_test(Source::Param(Source::T_rand, c_length_1K * 4, c_length_1K * 10), min_loop, handle);
		}
	}

}
}
#endif


