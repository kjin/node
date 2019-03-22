#ifndef SRC_TRACING_PERFETTO_NODE_TRACING_H_
#define SRC_TRACING_PERFETTO_NODE_TRACING_H_

#include "perfetto/tracing/core/tracing_service.h"
#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/producer.h"
#include "tracing/base/node_task_runner.h"
#include "tracing/base/node_consumer_endpoint.h"
#include "tracing/base/node_producer_endpoint.h"

#include <list>

using perfetto::TracingService;

namespace node {
namespace tracing {
namespace base {

class NodeTracing;

class NodeConsumer : public perfetto::Consumer {
 protected:
  inline perfetto::TracingService::ConsumerEndpoint* GetServiceEndpoint() { return svc_endpoint_.get(); }
  void OnConnect() override {}
  void OnDisconnect() override {}
  void OnTracingDisabled() override {}
  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override {}
  void OnDetach(bool success) override {}
  void OnAttach(bool success, const perfetto::TraceConfig&) override {}
  void OnTraceStats(bool success, const perfetto::TraceStats&) override {}
 private:
  std::unique_ptr<NodeConsumerEndpoint> svc_endpoint_;
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
  std::unique_ptr<NodeProducerEndpoint> svc_endpoint_;
  friend class NodeTracing;
};

/**
 * A wrapper around Perfetto's TracingService implementation.
 */
class NodeTracing {
 public:
  NodeTracing();
  ~NodeTracing();
  void Initialize();
  void ConnectConsumer(std::shared_ptr<NodeConsumer> consumer);
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
}

#endif
