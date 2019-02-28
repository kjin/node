#ifndef SRC_TRACING_PERFETTO_NODE_PRODUCER_H_
#define SRC_TRACING_PERFETTO_NODE_PRODUCER_H_

#include "perfetto/tracing/core/producer.h"
#include "perfetto/tracing/core/tracing_service.h"
#include "perfetto/tracing/core/data_source_config.h"
#include "perfetto/tracing/core/data_source_descriptor.h"
#include "tracing/agent.h"

namespace node {
namespace tracing {

class NodeProducer : public perfetto::Producer {
 public:
  NodeProducer();
  ~NodeProducer() {}
  TracingController* GetTracingController() { return tracing_controller_.get(); }
  void Connect(perfetto::TracingService* service);
 private:
  void OnConnect() override {
    PERFETTO_DLOG("Node producer connected");
    perfetto::DataSourceDescriptor ds_desc;
    ds_desc.set_name("node");
    svc_endpoint_->RegisterDataSource(ds_desc);
  }

  void OnDisconnect() override {
    PERFETTO_DLOG("Node producer disconnected");
  }

  void OnTracingSetup() override {
    PERFETTO_DLOG("Node producer OnTracingSetup(), registering data source");
  }

  void SetupDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig&) override {}

  void StartDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig& cfg) override {
    // target_buffer_ = cfg.target_buffer();
    PERFETTO_ILOG("Node perfetto producer: starting tracing for data source: %s",
                  cfg.name().c_str());
  }

  void StopDataSource(perfetto::DataSourceInstanceID) override {}

  void Flush(::perfetto::FlushRequestID,
             const perfetto::DataSourceInstanceID*, size_t) override {}

  std::unique_ptr<perfetto::TracingService::ProducerEndpoint> svc_endpoint_;
  std::unique_ptr<TracingController> tracing_controller_;
};

}
}

#endif
