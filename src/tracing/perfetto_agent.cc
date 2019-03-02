#include "tracing/perfetto_agent.h"
#include "tracing/perfetto/node_shared_memory.h"
#include "perfetto/tracing/core/data_source_descriptor.h"
#include "perfetto/tracing/core/trace_config.h"

using v8::platform::tracing::TraceObject;
using perfetto::base::TaskRunner;

namespace node {
namespace tracing {

class TestNodeConsumer : public NodeConsumer {
  void OnConnect() override {
    perfetto::TraceConfig config;
    svc_endpoint_->EnableTracing(config);
  }
};

class TraceControllerNodeProducer : public NodeProducer {
 public:
  TraceControllerNodeProducer(TracingController* trace_controller): trace_controller_(trace_controller) {}
 private:
  void OnConnect() override {
    printf("Connected\n");
    perfetto::DataSourceDescriptor ds_desc;
    ds_desc.set_name("node");
    svc_endpoint_->RegisterDataSource(ds_desc);
  }

  void OnDisconnect() override {
    printf("Disconnected\n");
  }

  void OnTracingSetup() override {
    printf("Tracing set up\n");
  }

  void SetupDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig&) override {
    printf("Data source set up\n");
  }

  void StartDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig& cfg) override {
    printf("Data source started\n");
  }
  TracingController* trace_controller_;
};

class NoopAgentWriterHandle : public AgentWriterHandle {
 public:
  NoopAgentWriterHandle(Agent* agent, std::unique_ptr<AsyncTraceWriter> writer, bool is_default): agent_(agent), writer_(std::move(writer)), is_default_(is_default) {}
  ~NoopAgentWriterHandle() {}
  bool empty() const override { return agent_ != nullptr; }
  void reset() override { agent_ = nullptr; }

  void Enable(const std::set<std::string>& categories) override {}
  void Disable(const std::set<std::string>& categories) override {}

  bool IsDefaultHandle() override { return is_default_; }

  Agent* agent() override { return agent_; }

  v8::TracingController* GetTracingController() override { return agent_->GetTracingController(); }
 private:
  Agent* agent_;
  std::unique_ptr<AsyncTraceWriter> writer_;
  bool is_default_;
};

// PerfettoAgent implementation

PerfettoAgent::PerfettoAgent()
  : node_tracing_(new NodeTracing()), tracing_controller_(new TracingController()) {
  tracing_controller_->Initialize(nullptr);
  producer_handle_ = node_tracing_->ConnectProducer(std::unique_ptr<NodeProducer>(
    new TraceControllerNodeProducer(tracing_controller_.get())));
  consumer_handle_ = node_tracing_->ConnectConsumer(std::unique_ptr<NodeConsumer>(new TestNodeConsumer()));
}

void PerfettoAgent::Initialize() {
  node_tracing_->Initialize();
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::AddClient(
  const std::set<std::string>& categories,
  std::unique_ptr<AsyncTraceWriter> writer,
  enum UseDefaultCategoryMode mode) {
    return std::unique_ptr<AgentWriterHandle>(new NoopAgentWriterHandle(this, std::move(writer), false));
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::DefaultHandle() {
  // Assume that this value can only be retrieved once.
  return std::unique_ptr<AgentWriterHandle>(new NoopAgentWriterHandle(this, std::unique_ptr<AsyncTraceWriter>(nullptr), true));
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
