#include "tracing/perfetto/node_consumer.h"

namespace node {
namespace tracing {

void NodeConsumer::Connect(perfetto::TracingService* service) {
  svc_endpoint_ = service->ConnectConsumer(this, 0);
}

void NodeConsumer::Disconnect() {
  svc_endpoint_.reset(nullptr);
}

DefaultNodeConsumer::DefaultNodeConsumer() {
  is_default_ = true;
  trace_config_.set_write_into_file(false);
}

void PerfettoConsumerHandle::Enable(const std::set<std::string>& categories) {
  // TODO(kjin): Actually, make a copy of the tracing config.
  if (consumer_->svc_endpoint_) {
    consumer_->svc_endpoint_->EnableTracing(consumer_->trace_config_);
  }
  // Maybe also throw an exception if not connected yet.
}

void PerfettoConsumerHandle::Disable(const std::set<std::string>& categories) {
  // TODO(kjin): Actually, make a copy of the tracing config.
  if (consumer_->svc_endpoint_) {
    consumer_->svc_endpoint_->DisableTracing();
  }
}

}
}
