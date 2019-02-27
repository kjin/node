#include "tracing/perfetto/node_consumer.h"

namespace node {
namespace tracing {

void NodeConsumer::Connect(perfetto::TracingService* service) {
  svc_endpoint_ = service->ConnectConsumer(this, 0);
}

std::unique_ptr<AgentWriterHandle> NodeConsumer::CreateHandle(Agent* agent) {
  return std::unique_ptr<AgentWriterHandle>(new PerfettoConsumerHandle(agent, svc_endpoint_.get()));
}

void PerfettoConsumerHandle::Enable(const std::set<std::string>& categories) {}

void PerfettoConsumerHandle::Disable(const std::set<std::string>& categories) {}

}
}
