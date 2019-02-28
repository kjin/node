#include "tracing/perfetto/node_consumer.h"

namespace node {
namespace tracing {

void NodeConsumer::Connect(perfetto::TracingService* service) {
  svc_endpoint_ = service->ConnectConsumer(this, 0);
}

DefaultNodeConsumer::DefaultNodeConsumer() {
  is_default_ = true;
  trace_config_.set_write_into_file(false);
}

void PerfettoConsumerHandle::Enable(const std::set<std::string>& categories) {
  // TODO(kjin): Actually, make a copy of the tracing config.
  consumer_->svc_endpoint_->EnableTracing(consumer_->trace_config_);
}

void PerfettoConsumerHandle::Disable(const std::set<std::string>& categories) {
  // TODO(kjin): Actually, make a copy of the tracing config.
  consumer_->svc_endpoint_->DisableTracing();
}

}
}
