

#include <stdio.h>
#include <stdlib.h>

#include "zookeeper/zookeeper.h"
#include "glog/logging.h" 

void global_watcher_fn(zhandle_t *zh, int type, int state,
                       const char *path, void *ctx);

class Zoo {
public:
	virtual ~Zoo() {
		zookeeper_close(handle);
	}

public:
    zhandle_t*	handle = NULL;
    std::string	servers = "192.168.10.111:2181,192.168.10.112:2181,192.168.10.113:2181";
    int 		timeout = 3000;
    const clientid_t* clientid = NULL;

public:
    int Connect() {
    	handle = zookeeper_init(servers.c_str(),
    		global_watcher_fn, timeout, clientid, this, 0);

    	if (handle == NULL) {
			LOG(WARNING) << "init zkhandle failed szhosts: " << servers; 
			return 1;

        } else {
            return 0;
		}
    }

    int establish() {
        int64_t old = clientid ? clientid->client_id : 0;

            if (clientid) {
                LOG(INFO) << "connect success, handle " << handle 
                    << ", clientid id " << clientid->client_id
                    << ", passwd " << clientid->passwd; 

            } else {
                LOG(INFO) << "connect success, handle " << handle;
            }

            // note: clientid will not filled rightly, wait some time
            clientid = zoo_client_id(handle);

            if (clientid->client_id != 0) {
                if (clientid->client_id == old) {
                    LOG(INFO) << "clientid unchange, id " << clientid->client_id
                        << ", passwd " << clientid->passwd; 

                } else {
                    LOG(INFO) << "clientid change id " << clientid->client_id
                        << ", passwd " << clientid->passwd; 
                }
            }
            return 0;
    }
};

inline void global_watcher_fn(zhandle_t *zh, int type, int state,
                       const char *path, void *ctx) {
    Zoo* zoo = (Zoo*)ctx;
    

    if (type == ZOO_CREATED_EVENT) {
        LOG(INFO) << "ZOO_CREATED_EVENT";
    } else if (type == ZOO_DELETED_EVENT) {
        LOG(INFO) << "ZOO_DELETED_EVENT";
    } else if (type == ZOO_CHANGED_EVENT) {
        LOG(INFO) << "ZOO_CHANGED_EVENT";
    } else if (type == ZOO_CHILD_EVENT) {
        LOG(INFO) << "ZOO_CHILD_EVENT";

    } else if (type == ZOO_SESSION_EVENT) {
        LOG(INFO) << "--------> type: " << type << ", state: " << state
              << ", path: " << path;

    } else if (type == ZOO_NOTWATCHING_EVENT) {
        LOG(INFO) << "ZOO_NOTWATCHING_EVENT";
    } else {
        LOG(INFO) << "Unknown type";
    }

    if (state == ZOO_EXPIRED_SESSION_STATE) {
        LOG(INFO) << "ZOO_EXPIRED_SESSION_STATE";
        zoo->Connect();

    } else if (state == ZOO_AUTH_FAILED_STATE) {
        LOG(INFO) << "ZOO_AUTH_FAILED_STATE";
    } else if (state == ZOO_CONNECTING_STATE) {
        LOG(INFO) << "ZOO_CONNECTING_STATE";

    } else if (state == ZOO_ASSOCIATING_STATE) {
        LOG(INFO) << "ZOO_ASSOCIATING_STATE";

    } else if (state == ZOO_CONNECTED_STATE) {
        // LOG(INFO) << "ZOO_CONNECTED_STATE";
        zoo->establish();

    } else {
        LOG(INFO) << "Unknown state";
    }

}

