#include "tracing/perfetto_agent.h"
#include "tracing/perfetto/node_shared_memory.h"

using v8::platform::tracing::TraceObject;
using perfetto::base::TaskRunner;

namespace node {
namespace tracing {

// PerfettoAgent implementation

PerfettoAgent::PerfettoAgent()
  : producer_(new NodeProducer()),
    task_runner_(new DelayedNodeTaskRunner()) {
  tracing_service_ = perfetto::TracingService::CreateInstance(
    std::unique_ptr<perfetto::SharedMemory::Factory>(new NodeShmemFactory()),
    task_runner_.get()
  );
  producer_->Connect(tracing_service_.get());
  std::shared_ptr<NodeConsumer> default_consumer(new DefaultNodeConsumer());
  default_consumer->Connect(tracing_service_.get());
  consumers_.push_back(default_consumer);
  consumer_handle_ = std::unique_ptr<PerfettoConsumerHandle>(new PerfettoConsumerHandle(default_consumer, this, task_runner_.get()));
}

PerfettoAgent::~PerfettoAgent() {
  // All producers and consumers must be disconnected first before the
  // tracing service instance can be destroyed.
  producer_.reset(nullptr);
  for (auto itr = consumers_.begin(); itr != consumers_.end(); itr++) {
    if (!itr->expired()) {
      std::shared_ptr<NodeConsumer> consumer(*itr);
      consumer->Disconnect();
    }
  }
  tracing_service_.reset(nullptr);
}

void PerfettoAgent::Initialize() {
  // The task runner can't enqueue tasks until the Node platform is initialized.
  // Connecting consumers and producers will cause the tracing service to
  // post tasks.
  task_runner_->Start();
}

TracingController* PerfettoAgent::GetTracingController() {
  return producer_->GetTracingController();
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::AddClient(
  const std::set<std::string>& categories,
  std::unique_ptr<AsyncTraceWriter> writer,
  enum UseDefaultCategoryMode mode) {
    std::shared_ptr<NodeConsumer> consumer = std::unique_ptr<NodeConsumer>(new AsyncTraceWriterConsumer(std::move(writer)));
    consumer->Connect(tracing_service_.get());
    consumers_.push_back(consumer);
    return std::unique_ptr<PerfettoConsumerHandle>(new PerfettoConsumerHandle(std::move(consumer), this, task_runner_.get()));
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::DefaultHandle() {
  // Assume that this value can only be retrieved once.
  CHECK(consumer_handle_);
  return std::move(consumer_handle_);
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
