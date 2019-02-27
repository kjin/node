#include "tracing/perfetto_agent.h"
#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/producer.h"
#include "perfetto/tracing/core/trace_packet.h"
#include "debug_utils.h"
#include <stdio.h>
#include "uv.h"

using v8::platform::tracing::TraceObject;
using perfetto::base::TaskRunner;

namespace node {
namespace tracing {

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

/**
 * An implementation of TaskRunner on the libuv thread pool.
 */
class NodeTaskRunner : public perfetto::base::TaskRunner {
 public:
  NodeTaskRunner() {
    CHECK_EQ(uv_loop_init(&tracing_loop_), 0);
  }

  ~NodeTaskRunner() {
    Stop();
    // TODO(kjin): Verify that this is for the sake of draining the queue...
    uv_run(&tracing_loop_, UV_RUN_ONCE);
    CheckedUvLoopClose(&tracing_loop_);
  }

  void PostTask(std::function<void()> task) override {
    uv_async_t async;
    CHECK_EQ(uv_async_init(&tracing_loop_, &async, [](uv_async_t* async) {
      std::function<void()> task = *static_cast<std::function<void()>*>(async->data);
      task();
    }), 0);
    // This probably won't work.
    async.data = &task;
    CHECK_EQ(uv_async_send(&async), 0);
  }

  void PostDelayedTask(std::function<void()> task, uint32_t delay_ms) override {
    uv_timer_t timer;
    CHECK_EQ(uv_timer_init(&tracing_loop_, &timer), 0);
    // This probably won't work.
    timer.data = &task;
    CHECK_EQ(uv_timer_start(&timer, [](uv_timer_t* timer) {
      std::function<void()> task = *static_cast<std::function<void()>*>(timer->data);
      task();
    }, delay_ms, 0), 0);
  }
  void AddFileDescriptorWatch(int fd, std::function<void()>) override {}
  void RemoveFileDescriptorWatch(int fd) override {}
  bool RunsTasksOnCurrentThread() const override { return false; }

  void Start() {
    uv_thread_create(&thread_, [](void* arg) {
      NodeTaskRunner* agent = static_cast<NodeTaskRunner*>(arg);
      uv_run(&agent->tracing_loop_, UV_RUN_DEFAULT);
    }, this);
  }

  void Stop() {
    uv_thread_join(&thread_);
  }
 private:
  uv_thread_t thread_;
  uv_loop_t tracing_loop_;
};

class DefaultNodeConsumer : public perfetto::Consumer {
 public:
  std::unique_ptr<TracingService::ConsumerEndpoint> svc_endpoint;

 private:
  void OnConnect() override { PERFETTO_DLOG("Default consumer connected"); }
  void OnDisconnect() override { PERFETTO_DLOG("Default consumer disconnected"); }
  void OnTracingDisabled() override {
    PERFETTO_DLOG("Default consumer trace disabled");
  }

  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override {
    PERFETTO_DLOG("Default consumer packets %zu %d", packets.size(), has_more);
  }

  void OnDetach(bool success) override {
    PERFETTO_DLOG("Default consumer OnDetach");
  }

  void OnAttach(bool success, const perfetto::TraceConfig&) override {
    PERFETTO_DLOG("Default consumer OnAttach");
  }

  void OnTraceStats(bool success, const perfetto::TraceStats&) override {
    PERFETTO_DLOG("Default consumer OnTraceStats");
  }
};

// PerfettoAgent implementation

PerfettoAgent::PerfettoAgent()
  : tracing_controller_(new TracingController()),
    task_runner_(new NodeTaskRunner()),
    default_consumer_(new DefaultNodeConsumer()) {
  tracing_controller_->Initialize(nullptr);
}

PerfettoAgent::~PerfettoAgent() {}

void PerfettoAgent::Start() {
  if (tracing_service_ == nullptr) { // == has been started
    task_runner_->Start();
    tracing_service_ = TracingService::CreateInstance(
      std::unique_ptr<perfetto::SharedMemory::Factory>(new NodeShmemFactory()),
      task_runner_.get()
    );
    default_consumer_->svc_endpoint = tracing_service_->ConnectConsumer(default_consumer_.get(), 0);
  }
}

void PerfettoAgent::Stop() {
  tracing_service_.reset(nullptr);
  task_runner_->Stop();
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
