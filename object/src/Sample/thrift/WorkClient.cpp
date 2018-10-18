
#include "WorkService.h"  // As an example

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>

#include <iostream>

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace test;
using namespace std;

int main(int argc, char **argv) {
	boost::shared_ptr<TSocket> socket(new TSocket("localhost", 9090));
	boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
	boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

	WorkServiceClient client(protocol);
	try {
		transport->open();
		int v = client.ping(20);
		cout << "ping recv: " << v << std::endl;

		WorkTask req, res;
		req.V64 = 100;
		req.Vstr = "message";

		client.trigger(res, 100, req);
		cout << "task recv: " << res.V64 << ", str: " << res.Vstr << std::endl;

		transport->close();
	} catch (TException& tx) {
		cout << "ERROR: " << tx.what() << endl;
	}
	return 0;
}
