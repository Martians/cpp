

#include "Test.hpp"
#include <assert.h>

int main(int argc, char *argv[])
{
    // google::InitGoogleLogging("output");
    // google::SetLogDestination(google::GLOG_INFO, "./output.log");

    Test a = Test();
    a.Connect();
    printf("success");
}
