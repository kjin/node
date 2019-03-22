#include "tracing/base/node_consumer_endpoint.h"

namespace node {
namespace tracing {
namespace base {

using FlushCallback = std::function<void(bool /*success*/)>;

NodeConsumerEndpoint::~NodeConsumerEndpoint() {}

void NodeConsumerEndpoint::EnableTracing(const perfetto::TraceConfig& config, perfetto::base::ScopedFile file) {
  task_runner_->PostTask([=, &file]() {
    svc_endpoint_->EnableTracing(config, std::move(file));
  });
}

void NodeConsumerEndpoint::StartTracing() {
  task_runner_->PostTask([=]() {
    svc_endpoint_->StartTracing();
  });
}

void NodeConsumerEndpoint::DisableTracing() {
  task_runner_->PostTask([=]() {
    svc_endpoint_->DisableTracing();
  });
}

void NodeConsumerEndpoint::Flush(uint32_t timeout_ms, FlushCallback callback) {
  task_runner_->PostTask([=]() {
    svc_endpoint_->Flush(timeout_ms, callback);
  });
}

void NodeConsumerEndpoint::ReadBuffers() {
  task_runner_->PostTask([=]() {
    svc_endpoint_->ReadBuffers();
  });
}

void NodeConsumerEndpoint::FreeBuffers() {
  task_runner_->PostTask([=]() {
    svc_endpoint_->FreeBuffers();
  });
}

void NodeConsumerEndpoint::Detach(const std::string& key) {
  task_runner_->PostTask([=]() {
    svc_endpoint_->Detach(key);
  });
}

void NodeConsumerEndpoint::Attach(const std::string& key) {
  task_runner_->PostTask([=]() {
    svc_endpoint_->Attach(key);
  });
}

void NodeConsumerEndpoint::GetTraceStats() {
  task_runner_->PostTask([=]() {
    svc_endpoint_->GetTraceStats();
  });
}


}
}
}
