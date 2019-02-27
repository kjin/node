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
  task_runner_->Start();
  tracing_service_ = perfetto::TracingService::CreateInstance(
    std::unique_ptr<perfetto::SharedMemory::Factory>(new NodeShmemFactory()),
    task_runner_.get()
  );
  producer_->Connect(tracing_service_.get());
  consumers_[kDefaultHandleId] = std::unique_ptr<NodeConsumer>(new DefaultNodeConsumer());
  consumers_[kDefaultHandleId]->Connect(tracing_service_.get());
}

PerfettoAgent::~PerfettoAgent() {}

TracingController* PerfettoAgent::GetTracingController() {
  return producer_->GetTracingController();
}

// TODO(kjin): Destroying the handle disconnects the client
std::unique_ptr<AgentWriterHandle> PerfettoAgent::AddClient(
  const std::set<std::string>& categories,
  std::unique_ptr<AsyncTraceWriter> writer,
  enum UseDefaultCategoryMode mode) {
    size_t id = next_writer_id_++;
    consumers_[id] = std::unique_ptr<NodeConsumer>(new AsyncTraceWriterConsumer(std::move(writer)));
    consumers_[id]->Connect(tracing_service_.get());
    return consumers_[id]->CreateHandle(this);
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::DefaultHandle() {
  return consumers_[kDefaultHandleId]->CreateHandle(this);
}

std::string PerfettoAgent::GetEnabledCategories() const { return ""; } // TODO

void PerfettoAgent::AppendTraceEvent(TraceObject* trace_event) {
  // This should never get called, because the Tracing Controller communicates
  // through Perfetto rather than through the Agent.
  UNREACHABLE();
}

void PerfettoAgent::AddMetadataEvent(std::unique_ptr<TraceObject> event) {
  // This should never get called, because the Tracing Controller communicates
  // through Perfetto rather than through the Agent.
  UNREACHABLE();
}

void PerfettoAgent::Flush(bool blocking) {
  // This should never get called, because the Tracing Controller communicates
  // through Perfetto rather than through the Agent.
  UNREACHABLE();
}

TraceConfig* PerfettoAgent::CreateTraceConfig() const {
  return new TraceConfig();
}

}
}
