#ifndef SRC_TRACING_PERFETTO_AGENT_H_
#define SRC_TRACING_PERFETTO_AGENT_H_

#include "perfetto/tracing/core/shared_memory.h"
#include "perfetto/tracing/core/tracing_service.h"
#include "perfetto/base/task_runner.h"
#include "tracing/agent.h"

using v8::platform::tracing::TraceConfig;
using v8::platform::tracing::TraceObject;
using node::tracing::AgentWriterHandle;
using node::tracing::AsyncTraceWriter;
using node::tracing::TracingController;
using node::tracing::UseDefaultCategoryMode;
using perfetto::TracingService;

namespace node {
namespace tracing {

class PerfettoAgent;
class NodeTaskRunner;
class DefaultNodeConsumer;

class PerfettoAgentWriterHandle : public AgentWriterHandle {
 public:
  inline PerfettoAgentWriterHandle() {}
  inline ~PerfettoAgentWriterHandle() { reset(); }

  inline bool empty() const override { return agent_ == nullptr; }
  inline void reset() override {}

  inline void Enable(const std::set<std::string>& categories) override {}
  inline void Disable(const std::set<std::string>& categories) override {}

  inline bool IsDefaultHandle() override { return false; }

  inline Agent* agent() override;

  inline v8::TracingController* GetTracingController() override;

  PerfettoAgentWriterHandle(const PerfettoAgentWriterHandle& other) = delete;
  PerfettoAgentWriterHandle& operator=(const PerfettoAgentWriterHandle& other) = delete;

 private:
  inline PerfettoAgentWriterHandle(PerfettoAgent* agent, int id) : agent_(agent), id_(id) {}

  PerfettoAgent* agent_ = nullptr;
  int id_;

  friend class PerfettoAgent;
};

class PerfettoAgent : public Agent {
 public:
  PerfettoAgent();
  ~PerfettoAgent();
  
  TracingController* GetTracingController() override;

  // Destroying the handle disconnects the client
  std::unique_ptr<AgentWriterHandle> AddClient(const std::set<std::string>& categories,
                              std::unique_ptr<AsyncTraceWriter> writer,
                              enum UseDefaultCategoryMode mode) override;
  // A handle that is only used for managing the default categories
  // (which can then implicitly be used through using `USE_DEFAULT_CATEGORIES`
  // when adding a client later).
  std::unique_ptr<AgentWriterHandle> DefaultHandle() override;

  // Returns a comma-separated list of enabled categories.
  std::string GetEnabledCategories() const override;

  // Writes to all writers registered through AddClient().
  void AppendTraceEvent(TraceObject* trace_event) override;

  void AddMetadataEvent(std::unique_ptr<TraceObject> event) override;
  // Flushes all writers registered through AddClient().
  void Flush(bool blocking) override;

  TraceConfig* CreateTraceConfig() const override;
 private:
  void Start();
  void Stop();

  std::unique_ptr<TracingService> tracing_service_;
  std::unique_ptr<TracingController> tracing_controller_;
  std::unique_ptr<NodeTaskRunner> task_runner_;
  std::unique_ptr<DefaultNodeConsumer> default_consumer_;
};

Agent* PerfettoAgentWriterHandle::agent() { return agent_; }

v8::TracingController* PerfettoAgentWriterHandle::GetTracingController() {
  return agent_->GetTracingController();
}

}
}

#endif