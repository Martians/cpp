
#include <cassert>
#include <algorithm>

#include "Common/Util.hpp"
#include "Perform/Random.hpp"

/** for dump test */
#include <Common/Display.hpp>
#include "Common/Logger.hpp"

namespace common {
namespace random {

#if 0
template <class GenType, class DisType>
class TypeRandom : public Random
{
public:
	TypeRandom(int seed, int generator, int distribution) {
		generator(seed, generator);
		distribution(type);
	}


	void 	generator(rand_t seed, int type) {

	}

	void 	distribution(int type) {

	}

protected:
	std::random_device rd;
	std::ranlux48 ranlux;

public:

	std::function<int64_t()> m_rand;
};

#endif
///**
// *
// **/
//template<class Type>
//class Generator
//{
//public:
//
//
//	Generator(int type)
//		: m_type(seed) {}
//
//	Type& type() { return m_type; }
//
//protected:
//	Type m_type;
//};
//
//template<class Type>
//class Distribution
//{
//public:
//
//	template<class Generator>
//	Distribution(Generator& gen, int min, int max)
//
//	switch (distribution) {
//		case Random::T_default: {
//
//		} break;
//
//		default:
//		break;
//	}
//	std::shared_ptr<std::uniform_int_distribution<int>> =
//
//
//	Type& type() { return m_type; }
//
//protected:
//	Type m_type;
//};

void
CharTable::reset(bool fill)
{
	m_size = 0;
	memset(m_data, 0, sizeof(m_data));

	if (fill) {
		for (byte_t c = c_char_min; c < c_char_max; c++) {
			update(c);
		}
	}
}

void
CharTable::expected(std::initializer_list<Range> array)
{
	byte_t data[c_char_max] = {0};
	for (auto& range : array) {
		assert(range.min <= range.max);
		for (byte_t curr = range.min; curr <= range.max; curr++) {
			data[(size_t)curr] = curr;
		}
	}

	reset(false);
	for (auto v : data) {
		if (v != 0) {
			update(v);
		}
	}
	assert(m_size > 0);
}

void
CharTable::unexpect(std::initializer_list<char> array)
{
	byte_t data[c_char_max] = {0};
	memcpy(data, m_data, c_char_max);
	for (auto curr: array) {
		for (auto& v : data) {
			if (v == curr) {
				v = 0;
			}
		}
	}

	reset(false);
	for (auto v : data) {
		if (v != 0) {
			update(v);
		}
	}
	assert(m_size > 0);
}

bool
CharTable::verify(byte_t& c)
{
	if (c < m_data[0] || c > m_data[m_size - 1]) {
		return false;
	}

	auto ptr = std::lower_bound(m_data, m_data + m_size, c);
	return ptr != NULL && *ptr == c;
}

bool
CharTable::verify(byte_t* data, length_t size)
{
	for (byte_t* ptr = data; ptr < data + size; ++ptr) {
		if (!verify(*ptr)) {
			return false;
		}
	}
	return true;
}

byte_t
CharTable::convert_index(byte_t v, bool reverse)
{
	if (reverse) {
		assert((size_t)v < m_size);
		v = m_data[(size_t)v];
        return v;

	} else {
        auto ptr = std::lower_bound(m_data, m_data + m_size, v);
        assert(ptr != NULL && *ptr == v);
        return ptr - m_data;
	}
}

bool
CharTable::next_index(char& c)
{
	if ((u_byte_t)c < m_size) {
		return true;
	} else {
		c = 0;
		return false;
	}
}

void
Random::Param::range(array_t& ranges, array_t& weight)
{
	piecewise.ranges = ranges;
	piecewise.weight = weight;

	/** do more things, valid param */
}

Random::Random(Param param)
	: m_param(param)
{
	m_engine = new std::default_random_engine(param.seed);

	if (param.piecewise.ranges.size() == 0) {
		m_uniform = new std::uniform_int_distribution<int64_t>();

	} else {
		PieceRange& piece = param.piecewise;
		m_piecewise = new std::piecewise_constant_distribution<float>(piece.ranges.begin(),
			piece.ranges.end(), piece.weight.begin());
		m_record.resize(piece.ranges.size());
	}
}

void
Random::record(rand_t number)
{
	array_t& ranges = m_param.piecewise.ranges;
	auto iter = std::lower_bound(ranges.begin(), ranges.end(), number);
	auto curr = iter == ranges.end() ?
		ranges.size() - 1 : iter - ranges.begin();
	while (number == ranges[curr]) {
		++curr;
	}
	++m_record[curr - 1];
}

std::string
Random::dump_piece(int64_t max_star)
{
	array_t& ranges = m_param.piecewise.ranges;
	array_t& weight = m_param.piecewise.weight;

	int64_t total = 0;
	for (auto iter : m_record) {
		total += iter;
	}
	if (!get_bit(T_record) || total == 0) {
		return "";
	}

	::std::stringstream ss;
	ss << "\n";
	for (size_t i = 0; i < m_record.size(); ++i) {
		if (i < weight.size() && weight[i] != 0) {
			ss << "[" << string_size(ranges[i], false) << "-" << (i + 1 != m_record.size() ? string_size(ranges[i + 1], false) : "") << ")" << ": "
				<< std::string(m_record[i] * max_star / total, '*') << "\t" << m_record[i] * max_star / total << "\n";
	    } else {
	    	assert(m_record[i] == 0);
	    }
    }

    ss << "\nrange:\t";
    for (auto it = ranges.begin(); it != ranges.end(); it++) {
        ss << *it << ",";
    }

    ss << "\nweight:\t";
    for (auto it = weight.begin(); it != weight.end(); it++) {
        ss << *it << ",";
    }

    ss << "\nresult:\t";
    for (auto it = m_record.begin(); it != m_record.end(); it++) {
        ss << *it << ",";
    }
    log_info(ss.str());
    //return ss.str();
    return "";
}

random_ptr
RandomFactory::get(Random::Param type)
{
	return std::make_shared<Random>(type);
}

}
}

#if COMMON_TEST

#include "Common/Container.hpp"
#include "Common/Display.hpp"
#include "Common/Logger.hpp"

namespace common {
namespace tester {

using namespace ::common::random;

void
piecewise_test(std::initializer_list<int64_t> ranges, std::initializer_list<int64_t> weight)
{
	Random::Param param;
	param.range(ranges, weight);

	random_ptr rand = RandomFactory::get(param);
	rand->set_bit(Random::T_record, 1);
	for (int i = 0; i < 10000; ++i) {
		rand->next_piece();
	}
	rand->dump_piece();
	//log_info();
}

void
random_test()
{
	piecewise_test({1, 3, 5}, {2, 8});
	piecewise_test({1, 3, 3,5, 10, 100}, {1,0, 1,0, 1});
	piecewise_test({1, 3, 3,5, 10, 100}, {10,0, 20,0, 30});
}

}
}
#endif
/**
 * generator:
  	minstd_rand
	minstd_rand0
	knuth_b     minstd_rand0 with shuffle_order_engine

	default_random_engine()
	mt19937
	mt19937_64

	ranlux24_base
	ranlux48_base
	ranlux24              ranlux24_base with discard_block_engine
	ranlux48              ranlux48_base with discard_block_engi

 	distribution:
  	bernoulli_distribution
	binomial_distribution
	geometry_distribution
	negative_biomial_distribution

	poisson_distribution
	exponential_distribution
	gamma_distribution
	weibull_distribution
	extreme_value_distribution

	normal_distribution
	chi_squared_distribution
	cauchy_distribution
	fisher_f_distribution
	student_t_distribution

	discrete_distribution
	piecewise_constant_distribution
	piecewise_linear_distribution
 **/
