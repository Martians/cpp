

#include "Zoo.hpp" 
#include <assert.h>

Zoo zoo;

void ReConnect() {
    zoo.Connect();
    
    // close zk, check event
    getchar();
    zoo.Connect();
}

void UseClientID() {
    zoo.Connect();
    
    sleep(1);
    Zoo zoo1;
    zoo1.clientid = zoo.clientid;    
    zoo1.Connect();
    getchar();

    zoo.Connect();
}

void ConnLimit() {
    for (int i = 0; i < 1000; i++) {
        Zoo *zoo = new Zoo();

        zoo->servers = "192.168.10.111:2181";
        LOG(INFO) << "count - " << i;
        if (zoo->Connect() != 0) {
            LOG(ERROR) << "Connect limit !";
            assert(0);
        }
    }
    sleep(30);
}

int main(int argc, char *argv[]) {
    
    // google::InitGoogleLogging("output");
    // google::SetLogDestination(google::GLOG_INFO, "./output.log");

    // ReConnect();    
    // UseClientID();
    ConnLimit();

//    getchar();
}

