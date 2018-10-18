
#pragma once

#include <gtest/gtest.h>

#include "Common/Logger.hpp"
#include "Perform/Mock.hpp"

/**
 * test listener
 **/
class MockListener : public ::testing::EmptyTestEventListener
{
public:
	virtual void OnTestStart(const ::testing::TestInfo& test_info) {
		log_info("");
		log_info("");
		log_info("===========================================================");
		log_info("[" << test_info.test_case_name() << " - " << test_info.name() << "]");
	}

//	virtual void OnTestEnd(const ::testing::TestInfo& test_info) {
//	}
};

/**
 * test environment
 **/
class MockEnvironment : public testing::Environment
{
public:
	MockEnvironment() {
		/** set wait flag */
		Testing(true);

    	set_log_level(TRACE, common::LOG_START);

    	g_mock.SetDumpEvent(true);
	}

protected:

    virtual void SetUp() {}

    virtual void TearDown() {}
};

/**
 * base object test
 **/
class MockBase : public testing::Test
{
public:
	MockBase() {
		/** reset mock info */
		g_mock.Reset();

		/** wait if set */
		g_mock.StepWait();
	}

protected:

    virtual void SetUp() {}

    virtual void TearDown() {}
};

namespace testing {
	/**
	 * execute all test suite, add some additional option
	 **/
	int		RunTest(int argc, char* argv[], testing::Environment* env = new MockEnvironment);
}
