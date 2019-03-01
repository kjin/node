#include "tracing/perfetto_agent.h"
#include "tracing/perfetto/node_shared_memory.h"
#include "perfetto/tracing/core/data_source_descriptor.h"

using v8::platform::tracing::TraceObject;
using perfetto::base::TaskRunner;

namespace node {
namespace tracing {

class TraceControllerNodeProducer : public NodeProducer {
  void OnConnect() override {
    PERFETTO_DLOG("Node producer connected");
    perfetto::DataSourceDescriptor ds_desc;
    ds_desc.set_name("node");
    svc_endpoint_->RegisterDataSource(ds_desc);
  }

  void OnDisconnect() override {
    PERFETTO_DLOG("Node producer disconnected");
  }

  void OnTracingSetup() override {
    PERFETTO_DLOG("Node producer OnTracingSetup(), registering data source");
  }

  void SetupDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig&) override {}

  void StartDataSource(perfetto::DataSourceInstanceID,
                       const perfetto::DataSourceConfig& cfg) override {
    // target_buffer_ = cfg.target_buffer();
    PERFETTO_ILOG("Node perfetto producer: starting tracing for data source: %s",
                  cfg.name().c_str());
  }
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
