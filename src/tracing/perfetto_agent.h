#ifndef SRC_TRACING_PERFETTO_AGENT_H_
#define SRC_TRACING_PERFETTO_AGENT_H_

#include "tracing/agent.h"
#include "tracing/perfetto/node_task_runner.h"
#include "tracing/perfetto/node_consumer.h"
#include "tracing/perfetto/node_producer.h"

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
  ~PerfettoAgent();

  void Initialize() override;
  
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

  void AddMetadataEvent(std::unique_ptr<TraceObject> event) override;
 private:
  // void Connect(NodeConsumer* consumer);
  
  // A Perfetto producer that owns the v8 TracingController. Any traces created
  // through the tracing controller are sent to the tracing service via this
  // producer.
  std::unique_ptr<NodeProducer> producer_;
  // A handle to the default Perfetto consumer. This consumer doesn't read any
  // trace data; it's only responsible for sending tracing configuration to
  // producers.
  // When DefaultHandle is called, ownership will be transferred there.
  std::unique_ptr<PerfettoConsumerHandle> consumer_handle_;
  // Weak pointers to consumers created by this agent, including the default
  // one. We need these pointers so that we can force disconnect all consumers
  // before tearing down the tracing service (see the destructor).
  std::list<std::weak_ptr<NodeConsumer>> consumers_;
  // A task runner with which the Perfetto service will post tasks. It runs all
  // tasks on the foreground thread.
  std::unique_ptr<DelayedNodeTaskRunner> task_runner_;
  // The Perfetto tracing service.
  std::unique_ptr<perfetto::TracingService> tracing_service_;
};

}
}

#endif