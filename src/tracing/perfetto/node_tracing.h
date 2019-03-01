#ifndef SRC_TRACING_PERFETTO_NODE_TRACING_H_
#define SRC_TRACING_PERFETTO_NODE_TRACING_H_

#include "perfetto/tracing/core/tracing_service.h"
#include "perfetto/tracing/core/trace_config.h"
#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/producer.h"
#include "tracing/perfetto/node_task_runner.h"

#include <list>

using perfetto::TracingService;

namespace node {
namespace tracing {

class NodeTracing;

class NodeConsumer : public perfetto::Consumer {
 protected:
  std::unique_ptr<perfetto::TracingService::ConsumerEndpoint> svc_endpoint_;
  perfetto::TraceConfig trace_config_;
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
  void Connect(perfetto::TracingService* service) {
    svc_endpoint_ = service->ConnectConsumer(this, 0);
  }
  bool IsConnected() {
    return !!svc_endpoint_;
  }
  void Disconnect() {
    svc_endpoint_.reset(nullptr);
  }
  friend class NodeTracing;
  friend class NodeConsumerHandle;
};

class NodeConsumerHandle {
 public:
  ~NodeConsumerHandle() {
    if (consumer_->IsConnected()) {
      consumer_->Disconnect();
      // Don't destroy the Consumer right away.
      std::shared_ptr<NodeConsumer> consumer = this->consumer_;
      task_runner_->PostTask([consumer]() {});
    }
  }
 private:
  NodeConsumerHandle(std::unique_ptr<NodeConsumer> consumer, NodeTaskRunner* task_runner): task_runner_(task_runner), consumer_(consumer.release()) {}
  NodeTaskRunner* task_runner_;
  std::shared_ptr<NodeConsumer> consumer_;
  friend class NodeTracing;
};

class NodeProducer : public perfetto::Producer {
 protected:
  std::unique_ptr<perfetto::TracingService::ProducerEndpoint> svc_endpoint_;
  //
  void OnConnect() override {}
  void OnDisconnect() override {}
  void OnTracingSetup() override {}
  void SetupDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig&) override {}
  void StartDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig& cfg) override {}
  void StopDataSource(perfetto::DataSourceInstanceID) override {}
  void Flush(::perfetto::FlushRequestID,
             const perfetto::DataSourceInstanceID*, size_t) override {}
 private:
  void Connect(perfetto::TracingService* service) {
    // TODO(kjin): Replace name.
    svc_endpoint_ = service->ConnectProducer(this, 0, "node");
  }
  bool IsConnected() {
    return !!svc_endpoint_;
  }
  void Disconnect() {
    svc_endpoint_.reset(nullptr);
  }
  friend class NodeTracing;
  friend class NodeProducerHandle;
};

class NodeProducerHandle {
 public:
  ~NodeProducerHandle() {
    if (producer_->IsConnected()) {
      producer_->Disconnect();
      // Don't destroy the Producer right away.
      std::shared_ptr<NodeProducer> producer = this->producer_;
      task_runner_->PostTask([producer]() {});
    }
  }
 private:
  NodeProducerHandle(std::unique_ptr<NodeProducer> producer, NodeTaskRunner* task_runner): task_runner_(task_runner), producer_(producer.release()) {}
  NodeTaskRunner* task_runner_;
  std::shared_ptr<NodeProducer> producer_;
  friend class NodeTracing;
};

class NodeTracing {
 public:
  NodeTracing();
  ~NodeTracing();
  void Initialize();
  std::unique_ptr<NodeConsumerHandle> ConnectConsumer(std::unique_ptr<NodeConsumer> consumer);
  std::unique_ptr<NodeProducerHandle> ConnectProducer(std::unique_ptr<NodeProducer> producer);
 private:
  std::list<std::weak_ptr<NodeProducer>> producers_;
  std::list<std::weak_ptr<NodeConsumer>> consumers_;
  // A task runner with which the Perfetto service will post tasks. It runs all
  // tasks on the foreground thread.
  std::unique_ptr<NodeTaskRunner> task_runner_;
  // The Perfetto tracing service.
  std::unique_ptr<perfetto::TracingService> tracing_service_;
};

}
}

#endif
