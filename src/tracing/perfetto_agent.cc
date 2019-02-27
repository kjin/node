#include "tracing/perfetto_agent.h"
#include <stdio.h>

using v8::platform::tracing::TraceObject;

class NodeShmem : public perfetto::SharedMemory {
 public:
  explicit NodeShmem(size_t size) : size_(size) {
    // Use valloc to get page-aligned memory. Some perfetto internals rely on
    // that for madvise()-ing unused memory.
    start_ = valloc(size);
  }

  ~NodeShmem() override { free(start_); }

  void* start() const override { return start_; }
  size_t size() const override { return size_; }

 private:
  void* start_;
  size_t size_;
};

class NodeShmemFactory : public perfetto::SharedMemory::Factory {
 public:
  ~NodeShmemFactory() override = default;
  std::unique_ptr<perfetto::SharedMemory> CreateSharedMemory(
      size_t size) override {
    return std::unique_ptr<perfetto::SharedMemory>(new NodeShmem(size));
  }
};

class NodeTaskRunner : public perfetto::base::TaskRunner {
 public:
  NodeTaskRunner() {}
  ~NodeTaskRunner() {}
  void PostTask(std::function<void()>) override {}
  void PostDelayedTask(std::function<void()>, uint32_t delay_ms) override {}
  void AddFileDescriptorWatch(int fd, std::function<void()>) override {}
  void RemoveFileDescriptorWatch(int fd) override {}
  bool RunsTasksOnCurrentThread() const override { return false; }
};

// PerfettoAgent implementation

namespace node {
namespace tracing {

PerfettoAgent::PerfettoAgent()
  : tracing_controller_(new TracingController()),
    task_runner_(new NodeTaskRunner()) {}

PerfettoAgent::~PerfettoAgent() {}

void PerfettoAgent::Start() {
  if (tracing_service_ == nullptr) {
    tracing_service_ = TracingService::CreateInstance(
      std::unique_ptr<perfetto::SharedMemory::Factory>(new NodeShmemFactory()),
      task_runner_.get()
    );
  }
}

void PerfettoAgent::Stop() {
  tracing_service_.reset(nullptr);
}

TracingController* PerfettoAgent::GetTracingController() {
  return tracing_controller_.get();
}

// Destroying the handle disconnects the client
std::unique_ptr<AgentWriterHandle> PerfettoAgent::AddClient(
  const std::set<std::string>& categories,
  std::unique_ptr<AsyncTraceWriter> writer,
  enum UseDefaultCategoryMode mode) {
    return std::unique_ptr<AgentWriterHandle>(new PerfettoAgentWriterHandle(this, 0));
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::DefaultHandle() {
  return std::unique_ptr<AgentWriterHandle>(new PerfettoAgentWriterHandle(this, 0));
}

std::string PerfettoAgent::GetEnabledCategories() const { return ""; } // TODO

void PerfettoAgent::AppendTraceEvent(TraceObject* trace_event) {}

void PerfettoAgent::AddMetadataEvent(std::unique_ptr<TraceObject> event) {}

void PerfettoAgent::Flush(bool blocking) {}

TraceConfig* PerfettoAgent::CreateTraceConfig() const {
  return new TraceConfig();
}

}
}
