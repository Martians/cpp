// This autogenerated skeleton file illustrates one way to adapt a synchronous
// interface into an asynchronous interface. You should copy it to another
// filename to avoid overwriting it and rewrite as asynchronous any functions
// that would otherwise introduce unwanted latency.

#include "WorkService.h"
#include <thrift/protocol/TBinaryProtocol.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::async;

using boost::shared_ptr;

using namespace  ::test;

class WorkServiceAsyncHandler : public WorkServiceCobSvIf {
 public:
  WorkServiceAsyncHandler() {
    syncHandler_ = std::auto_ptr<WorkServiceHandler>(new WorkServiceHandler);
    // Your initialization goes here
  }
  virtual ~WorkServiceAsyncHandler();

  void ping(tcxx::function<void(int32_t const& _return)> cob, const int32_t value) {
    int32_t _return = 0;
    _return = syncHandler_->ping(value);
    return cob(_return);
  }

  void trigger(tcxx::function<void(WorkTask const& _return)> cob, const int64_t seqno, const WorkTask& task) {
    WorkTask _return;
    syncHandler_->trigger(_return, seqno, task);
    return cob(_return);
  }

 protected:
  std::auto_ptr<WorkServiceHandler> syncHandler_;
};
