#include "tracing/perfetto_agent.h"
#include "tracing/perfetto/node_shared_memory.h"

using v8::platform::tracing::TraceObject;
using perfetto::base::TaskRunner;

namespace node {
namespace tracing {

// PerfettoAgent implementation

PerfettoAgent::PerfettoAgent()
  : producer_(new NodeProducer()),
    task_runner_(new NodeTaskRunner()) {
  tracing_service_ = perfetto::TracingService::CreateInstance(
    std::unique_ptr<perfetto::SharedMemory::Factory>(new NodeShmemFactory()),
    task_runner_.get()
  );
  producer_->Connect(tracing_service_.get());
  std::unique_ptr<NodeConsumer> default_consumer = std::unique_ptr<NodeConsumer>(new DefaultNodeConsumer());
  default_consumer->Connect(tracing_service_.get());
  default_consumer_handle_ = std::unique_ptr<PerfettoConsumerHandle>(new PerfettoConsumerHandle(std::move(default_consumer), this));
}

TracingController* PerfettoAgent::GetTracingController() {
  return producer_->GetTracingController();
}

// TODO(kjin): Destroying the handle disconnects the client
std::unique_ptr<AgentWriterHandle> PerfettoAgent::AddClient(
  const std::set<std::string>& categories,
  std::unique_ptr<AsyncTraceWriter> writer,
  enum UseDefaultCategoryMode mode) {
    std::unique_ptr<NodeConsumer> consumer = std::unique_ptr<NodeConsumer>(new AsyncTraceWriterConsumer(std::move(writer)));
    consumer->Connect(tracing_service_.get());
    return std::unique_ptr<PerfettoConsumerHandle>(new PerfettoConsumerHandle(std::move(consumer), this));
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::DefaultHandle() {
  // Assume that this value can only be retrieved once.
  CHECK(default_consumer_handle_);
  return std::move(default_consumer_handle_);
}

std::string PerfettoAgent::GetEnabledCategories() const { return ""; } // TODO

void PerfettoAgent::AddMetadataEvent(std::unique_ptr<TraceObject> event) {
  // This is an artifact of the way the tracing controller interacts with
  // the agent.
  // Ideally, this would not be on the Agent interface at all.
  // TODO(kjin): Implement this.
}

}
}
