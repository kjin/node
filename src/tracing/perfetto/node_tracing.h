#ifndef SRC_TRACING_PERFETTO_NODE_TRACING_H_
#define SRC_TRACING_PERFETTO_NODE_TRACING_H_

#include "perfetto/tracing/core/tracing_service.h"
#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/producer.h"
#include "tracing/perfetto/node_task_runner.h"
#include "tracing/node_producer_endpoint.h"

#include <list>

using perfetto::TracingService;

namespace node {
namespace tracing {

class NodeTracing;

class NodeConsumer : public perfetto::Consumer {
 protected:
  std::unique_ptr<perfetto::TracingService::ConsumerEndpoint> svc_endpoint_;
  std::weak_ptr<perfetto::base::TaskRunner> task_runner_;
  virtual void BeforeConnect() {}
  virtual void BeforeDisconnect() {}
  //
  void OnConnect() override {}
  void OnDisconnect() override {}
  void OnTracingDisabled() override {}
  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override {}
  void OnDetach(bool success) override {}
  void OnAttach(bool success, const perfetto::TraceConfig&) override {}
  void OnTraceStats(bool success, const perfetto::TraceStats&) override {}
 private:
  // Private members for use by friends only.
  void Connect(perfetto::TracingService* service) {
    BeforeConnect();
    svc_endpoint_ = service->ConnectConsumer(this, 0);
  }
  void Disconnect() {
    BeforeDisconnect();
    svc_endpoint_.reset(nullptr);
  }
  void SetTaskRunner(std::weak_ptr<perfetto::base::TaskRunner> task_runner) {
    task_runner_ = task_runner;
  }
  bool IsConnected() {
    return !!svc_endpoint_;
  }
  friend class NodeTracing;
  friend class NodeConsumerHandle;
};

class NodeConsumerHandle {
 public:
  ~NodeConsumerHandle() {
    if (consumer_->IsConnected()) {
      consumer_->Disconnect();
      // The task runner is owned by the NodeTracing instance that created this
      // handle, and will be destroyed with it. Because the NodeTracing instance
      // will also disconnect any remaining Consumer and Producer instances in
      // its destructor, then it's impossible for this reference to be expired.
      if (task_runner_.expired()) {
        UNREACHABLE();
      }
      // Don't destroy the Consumer right away.
      std::shared_ptr<NodeConsumer> consumer = consumer_;
      std::shared_ptr<perfetto::base::TaskRunner>(task_runner_)->PostTask([consumer]() {});
    }
  }
 private:
  NodeConsumerHandle(
    std::unique_ptr<NodeConsumer> consumer,
    std::shared_ptr<perfetto::base::TaskRunner> task_runner): task_runner_(task_runner), consumer_(consumer.release()) {
      consumer_->SetTaskRunner(task_runner_);
    }
  std::weak_ptr<perfetto::base::TaskRunner> task_runner_;
  std::shared_ptr<NodeConsumer> consumer_;
  friend class NodeTracing;
};

class NodeProducer : public perfetto::Producer {
 protected:
  inline perfetto::TracingService::ProducerEndpoint* GetServiceEndpoint() { return svc_endpoint_.get(); }
  void OnConnect() override {}
  void OnDisconnect() override {}
  void OnTracingSetup() override {}
  void SetupDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig&) override {}
  void StartDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig& cfg) override {}
  void StopDataSource(perfetto::DataSourceInstanceID) override {}
  void Flush(perfetto::FlushRequestID,
             const perfetto::DataSourceInstanceID*, size_t) override {}
 private:
  std::unique_ptr<base::NodeProducerEndpoint> svc_endpoint_;
  friend class NodeTracing;
  // friend class NodeProducerHandle;
};

// class NodeProducerHandle {
//  public:
//   ~NodeProducerHandle() {
//     if (producer_->IsConnected()) {
//       producer_->Disconnect();
//       // The task runner is owned by the NodeTracing instance that created this
//       // handle, and will be destroyed with it. Because the NodeTracing instance
//       // will also disconnect any remaining Consumer and Producer instances in
//       // its destructor, then it's impossible for this reference to be expired.
//       if (task_runner_.expired()) {
//         UNREACHABLE();
//       }
//       // Don't destroy the Producer right away.
//       std::shared_ptr<NodeProducer> producer = producer_;
//       std::shared_ptr<perfetto::base::TaskRunner>(task_runner_)->PostTask([producer]() {});
//     }
//   }
//  private:
//   NodeProducerHandle(
//     std::unique_ptr<NodeProducer> producer,
//     std::shared_ptr<perfetto::base::TaskRunner> task_runner): task_runner_(task_runner), producer_(producer.release()) {
//       producer_->SetTaskRunner(task_runner_);
//     }
//   std::weak_ptr<perfetto::base::TaskRunner> task_runner_;
//   std::shared_ptr<NodeProducer> producer_;
//   friend class NodeTracing;
// };

/**
 * A wrapper around Perfetto's TracingService implementation.
 */
class NodeTracing {
 public:
  NodeTracing();
  ~NodeTracing();
  void Initialize();
  std::unique_ptr<NodeConsumerHandle> ConnectConsumer(std::unique_ptr<NodeConsumer> consumer);
  void ConnectProducer(std::shared_ptr<NodeProducer> producer, std::string name);
 private:
  std::list<std::weak_ptr<NodeProducer>> producers_;
  std::list<std::weak_ptr<NodeConsumer>> consumers_;
  // A task runner with which the Perfetto service will post tasks. It runs all
  // tasks on the foreground thread.
  std::shared_ptr<NodeTaskRunner> task_runner_;
  // The Perfetto tracing service.
  std::unique_ptr<perfetto::TracingService> tracing_service_;
};

}
}

#endif
