// #ifndef SRC_TRACING_PERFETTO_AGENT_H_
// #define SRC_TRACING_PERFETTO_AGENT_H_

// #include "tracing/agent.h"
// #include "tracing/base/node_task_runner.h"
// #include "tracing/base/node_tracing.h"

// #include "v8.h"
// #include "perfetto/tracing/core/tracing_service.h"

// #include <list>

// using v8::platform::tracing::TraceConfig;
// using v8::platform::tracing::TraceObject;
// using node::tracing::AgentWriterHandle;
// using node::tracing::AsyncTraceWriter;
// using node::tracing::TracingController;
// using node::tracing::UseDefaultCategoryMode;

// namespace node {
// namespace tracing {

// /**
//  * In order to be accepted as a client of PerfettoAgent, a Perfetto Consumer
//  * additionally needs to expose methods to allow it to be enabled and disabled
//  * externally.
//  */
// class TracingAgentClientConsumer : public base::NodeConsumer {
//  public:
//   virtual void Enable(const std::set<std::string>& categories) = 0;
//   virtual void Disable(const std::set<std::string>& categories) = 0;
// };

// class PerfettoAgent : public Agent {
//  public:
//   PerfettoAgent();
//   ~PerfettoAgent() {}

//   void Initialize() override;

//   TracingController* GetTracingController() override {
//     return tracing_controller_.get();
//   }

//   // For interface compatibility purposes; this method actually does nothing.
//   std::unique_ptr<AgentWriterHandle> AddClient(const std::set<std::string>& categories,
//     std::unique_ptr<AsyncTraceWriter> writer,
//     enum UseDefaultCategoryMode mode) override;
//   std::unique_ptr<AgentWriterHandle> DefaultHandle() override;
//   // Operational method for adding a client.
//   std::unique_ptr<AgentWriterHandle> AddClient(
//     std::unique_ptr<TracingAgentClientConsumer> consumer);

//   // Returns a comma-separated list of enabled categories.
//   std::string GetEnabledCategories() const override;

//   void AddMetadataEvent(std::unique_ptr<TraceObject> event) override;
//  private:
//   std::unique_ptr<base::NodeTracing> node_tracing_;
//   std::shared_ptr<TracingController> tracing_controller_;
//   std::shared_ptr<base::NodeProducer> producer_;
// };

// }
// }

// #endif