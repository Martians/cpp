
#include "Common/Display.hpp"
#include "Common/Logger.hpp"
#include "Perform/StatThread.hpp"

namespace common {
namespace tester {

::std::string
format_simple(const char * fmt,...)
{
	FORMAT(fmt);
	return data;
}

void
StatisticThread::output(bool summary)
{
	int64_t iops = m_statis.iops.last();
	int64_t size = m_statis.size.last();
	int64_t iops_total = m_statis.iops.total();
	int64_t size_total = m_statis.size.total();
	int64_t wait = waited_time();
	auto time = m_timer.elapse();

	if (summary) {
		var_info("Empty: %3d total: %s size: %s, using: %s   iops: %s throughput: %s  error: %s", empty_total(),
			string_count(iops_total).c_str(),
			string_size(size_total).c_str(),
			string_elapse(time).c_str(),
			string_iops(iops_total, time).c_str(),
			string_speed(size_total, time).c_str(),
			string_count(m_statis.warn.total()).c_str());

	} else {
		var_info("%s iops: %6s,  lan: %9s,  output: %10s"
			"   total: %6s, size: %8s,  iops: %5s, speed: %s [%s]",
			string_elapse(time).c_str(),
			string_iops(iops, wait).c_str(),
			string_latancy(iops, wait).c_str(),
			string_speed(size, wait).c_str(),

			string_count(iops_total).c_str(),
			string_size(size_total).c_str(),
			string_iops(iops_total, time).c_str(),
			string_speed(size_total, time).c_str(),
			string_percent(iops_total, m_statis.total).c_str());
	}
}

void
StatisticThread::record(bool set)
{
	/** stop previous loop */
	if (m_statis.iops.total() ||
		m_statis.iops.count())
	{
		m_timer.check();
		int64_t iops = 0;
		int64_t size = 0;
		int64_t warn = 0;
		m_statis.Loop(iops, size, warn);

		summary(true);
		log_info("");
	}

	m_timer.begin();
	set_bit(T_record, set);
}

void
StatisticThread::active(int64_t iops)
{
	if (iops == 0) {
		set_bit(T_output, false);
		m_empty.inc();

	} else {
		set_bit(T_output, true);
		m_empty.loop();
	}
}

void*
StatisticThread::entry()
{
	while (waiting()) {

		if (get_bit(T_record)) {
			display();

			summary();
		}
	}
	summary(true);
	return NULL;
}

void
StatisticThread::display()
{
	int64_t iops = 0;
	int64_t size = 0;
	int64_t warn = 0;

	m_timer.check();
	m_statis.Loop(iops, size, warn);

	if (iops > 0) {
		if (m_output.empty()) {
			output(false);

		} else {
			log_info(m_output.work());
		}

	} else if (get_bit(T_empty)) {
		var_info("empty: %2d ... ", m_empty.count());
	}
	active(iops);
}

void
StatisticThread::summary(bool force)
{
	if (force || (get_bit(T_record) && m_check.expired())) {
		/** when force, not update summary time */
		if (force) {
			m_check.check();
		}

		log_info("=================================================");
		if (m_summary.empty()) {
			output(true);

		} else {
			log_info(m_summary.work());
		}
		log_info("=================================================");
	}
}

void
StatisticThread::regist()
{
	auto iops = BIND(&m_statis.iops, last);
	auto size = BIND(&m_statis.size, last);
	auto iops_total = BIND(&m_statis.iops, total);
	auto size_total = BIND(&m_statis.size, total);
	auto last = BIND_THIS(waited_time);
	auto time = BIND(&m_timer, elapse);

	add("%s ", string_elapse, time);
	add("iops: %6s,  ", string_iops, iops, last);
	add("lan: %9s,  ", string_latancy, iops, last);
	add("output: %10s", string_speed, size, last);
    
	add("   total: %6s, ", string_count, iops_total, true);
	add("size: %8s,  ", string_size, size_total, true);
	add("iops: %5s, ", string_iops, iops_total, time);
	add("speed: %s ", string_speed, size_total, time);
	add("[%s]", string_percent, iops_total, m_statis.total);

	sum("Empty: %s ", format_simple, "%3d", BIND_THIS(empty_total));
	sum("total: %s ", string_count, iops_total, true);
	sum("size: %s, ", string_size, size_total, true);
	sum("using: %s   ", string_elapse, time);
	sum("iops: %s ", string_iops, iops_total, time);
	sum("throughput: %s  ", string_speed, size_total, time);
	sum("error: %s", string_count, BIND(&m_statis.warn, total), true);

#if 0
{
    typedef std::function<int64_t()> value_t;
    typedef std::function<std::string(int, int)> output_t;
	auto iops = m_statis.iops.hd_last();
	value_t h1 = m_statis.iops.hd_last();
	log_info(typeid(iops).name() << "-" << typeid(h1).name());

	output_t b = std::bind(string_iops, h1, last);
	auto task = std::bind(&OutputList::wrap, &m_output, "%s", b);
	add("iops: %6s,  ", string_iops, h1, last);
}
#endif
}

}
}

#if COMMON_TEST

namespace common {
namespace tester {

void
statistic_test()
{
	StatisUnit st;

	OutputList out;
    auto b = BIND(&st, count);
	out.add("test %s", &string_count, b, false);

	st.inc(100);
	out.work();

	st.inc(300);
	out.work();
}

}
}
#endif
