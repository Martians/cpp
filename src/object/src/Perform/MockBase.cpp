
#include <stdio.h>
#include <iostream>

#include "Common/Mutex.hpp"
#include "Common/Logger.hpp"
#include "Common/Display.hpp"

#include "Perform/Mock.hpp"
#include "Perform/MockBase.hpp"

namespace mock {
	const char* c_not_exist_host = "1.1.1.1";
	const char* c_not_exist_value = "not_exist_value";
	const char* c_duplicate_value = "duplicate_value";
}

void
BaseMock::StepWait()
{
	if (mStep) {
		getchar();
	}
}

void
BaseMock::DumpWaitEvent()
{
	int key = 0;
	int status = 0;

    common::Mutex::Locker lock(mWaitEvent.mutex());

	mWaitEvent.init_wait(true);
	while ((status = mWaitEvent.next_wait(key, true)) > 0) {
		log_debug("\tMock: unexpect event " << key << " count " << status);
	}

	mWaitEvent.init_wait();
	while ((status = mWaitEvent.next_wait(key)) > 0) {
		log_debug("\tMock: still wait for event " << key << " count " << status);
	}
}

void
RegistListener(::testing::TestEventListener* listener = new MockListener)
{
    ::testing::TestEventListeners& listeners =
          ::testing::UnitTest::GetInstance()->listeners();
     //delete listeners.Release(listeners.default_result_printer());
     listeners.Append(listener);
}

char
single_param(const char* param) {
	if (strlen(param) == 2 && param[0] == '-') {
		return param[1];
	} else {
		return 0;
	}
}

bool
filter_param(int argc, char* argv[], const std::string& command)
{
	assert(optind <= argc);
	if (single_param(argv[optind - 1])) {
		return true;

	} else if (optind > 2) {
		std::string param(1, single_param(argv[optind - 2]));
		param += ":";
		return command.find(param) != std::string::npos;
	}
	return false;
}

int
::testing::RunTest(int argc, char* argv[], testing::Environment* env)
{
    char ch;
    opterr = 0;
    char data[c_length_1K] = {};

    const char* command = "df:wshe";
    while ((ch = getopt(argc, argv, command)) != -1) {
    	if (!filter_param(argc, argv, command)) {
    		continue;
    	}
        switch(ch) {
        case 'd': {
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

        } break;
        case 'f': {
        	snprintf(data, sizeof(data), "--gtest_filter=*%s*", optarg);
        	argv[optind - 1] = data;

        } break;
        case 'w': {
        	g_mock.SetDumpEvent(true);

        }break;
        case 's': {
        	g_mock.SetStep(true);

        } break;
        case 'h': {
        	prints("additional option help: \n"
        		 << "    -d          daemon mode disable stdout\n"
				 << "    -f <string> filter surround by * \n"
				 << "    -w          dump un-happen event \n"
				 << "    -s          step each test suite \n"
				 << "    -e          show this help and exit \n");
            //exit(-1);
        } break;
        case 'e': {
        	exit(0);

        } break;
        case '?':
			break;
        }
    }

    /** all argument will also transfer to gtest and parse again */
    testing::AddGlobalTestEnvironment(env);
    testing::InitGoogleTest(&argc, argv);
    RegistListener();
    return RUN_ALL_TESTS();
}

