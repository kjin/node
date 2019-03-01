#ifndef SRC_TRACING_PERFETTO_AGENT_H_
#define SRC_TRACING_PERFETTO_AGENT_H_

#include "tracing/agent.h"
#include "tracing/perfetto/node_task_runner.h"
#include "tracing/perfetto/node_tracing.h"

#include "v8.h"
#include "perfetto/tracing/core/tracing_service.h"

#include <list>

using v8::platform::tracing::TraceConfig;
using v8::platform::tracing::TraceObject;
using node::tracing::AgentWriterHandle;
using node::tracing::AsyncTraceWriter;
using node::tracing::TracingController;
using node::tracing::UseDefaultCategoryMode;

namespace node {
namespace tracing {

class PerfettoAgent : public Agent {
 public:
  PerfettoAgent();
  ~PerfettoAgent() {}

  void Initialize() override;
  
  TracingController* GetTracingController() override {
    return tracing_controller_.get();
  }

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

  void AddMetadataEvent(std::unique_ptr<TraceObject> event) override;
 private:
  std::unique_ptr<NodeTracing> node_tracing_;
  std::shared_ptr<TracingController> tracing_controller_;
};

}
}

#endif