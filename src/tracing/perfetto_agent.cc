#include "tracing/perfetto_agent.h"
#include "tracing/perfetto/node_shared_memory.h"
#include "tracing/perfetto/tracing_controller.h"

using v8::platform::tracing::TraceObject;
using perfetto::base::TaskRunner;
using perfetto::protos::pbzero::TracePacket;

namespace node {
namespace tracing {

class TestNodeConsumer : public NodeConsumer {
  void OnConnect() override {
    perfetto::TraceConfig config;
    config.add_buffers()->set_size_kb(4096);
    {
      auto c = config.add_data_sources();
      auto c2 = c->mutable_config();
      c2->set_name("trace_events");
    }
    config.set_duration_ms(1000);
    svc_endpoint_->EnableTracing(config);
  }
  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override {
    printf("Got trace data.\n");
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
  : node_tracing_(new NodeTracing()) {
  TracingControllerNodeProducer* producer = new TracingControllerNodeProducer();
  tracing_controller_ = producer->GetTracingController();
  producer_handle_ = node_tracing_->ConnectProducer(std::unique_ptr<NodeProducer>(producer));
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
