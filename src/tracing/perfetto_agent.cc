#include "tracing/perfetto_agent.h"
#include "tracing/perfetto/tracing_controller_producer.h"

using v8::platform::tracing::TraceObject;
using perfetto::base::TaskRunner;
using perfetto::protos::pbzero::TracePacket;

namespace node {
namespace tracing {

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

class TracingAgentClientConsumerHandle : public AgentWriterHandle {
 public:
  ~TracingAgentClientConsumerHandle() {}
  bool empty() const override { return consumer_handle_.get() == nullptr; }
  void reset() override {
    consumer_handle_.reset(nullptr);
    agent_ = nullptr;
    consumer_ = nullptr;
  }

  void Enable(const std::set<std::string>& categories) override {
    if (consumer_ == nullptr) {
      return;
    }
    consumer_->Enable(categories);
  }

  void Disable(const std::set<std::string>& categories) override {
    if (consumer_ == nullptr) {
      return;
    }
    consumer_->Disable(categories);
  }

  bool IsDefaultHandle() override { return false; }

  Agent* agent() override { return agent_; }

  v8::TracingController* GetTracingController() override { return agent_->GetTracingController(); }
 private:
  TracingAgentClientConsumerHandle(
    Agent* agent, TracingAgentClientConsumer* consumer, 
    std::unique_ptr<NodeConsumerHandle> consumer_handle)
    : agent_(agent), consumer_(consumer),
      consumer_handle_(std::move(consumer_handle)) {}
  // Reference to the Perfetto Agent.
  Agent* agent_;
  // Sub-class reference to the NodeConsumer that is owned by consumer_handle_.
  TracingAgentClientConsumer* consumer_;
  // Handle that owns the NodeConsumer.
  std::unique_ptr<NodeConsumerHandle> consumer_handle_;
  friend class PerfettoAgent;
};

// PerfettoAgent implementation

PerfettoAgent::PerfettoAgent()
  : node_tracing_(new NodeTracing()) {
  TracingControllerProducer* producer = new TracingControllerProducer();
  tracing_controller_ = producer->GetTracingController();
  producer_handle_ = node_tracing_->ConnectProducer(std::unique_ptr<NodeProducer>(producer));
}

void PerfettoAgent::Initialize() {
  node_tracing_->Initialize();
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::AddClient(
  const std::set<std::string>& categories,
  std::unique_ptr<AsyncTraceWriter> writer,
  enum UseDefaultCategoryMode mode) {
    // This method is essentially a no-op.
    return std::unique_ptr<AgentWriterHandle>(new NoopAgentWriterHandle(this, std::move(writer), false));
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::DefaultHandle() {
  // This method is essentially a no-op.
  return std::unique_ptr<AgentWriterHandle>(new NoopAgentWriterHandle(this, std::unique_ptr<AsyncTraceWriter>(nullptr), true));
}

std::unique_ptr<AgentWriterHandle> PerfettoAgent::AddClient(
  std::unique_ptr<TracingAgentClientConsumer> consumer) {
    std::unique_ptr<AgentWriterHandle> result = std::unique_ptr<AgentWriterHandle>(
      new TracingAgentClientConsumerHandle(this, consumer.get(), node_tracing_->ConnectConsumer(std::move(consumer))));
    result->Enable(std::set<std::string>());
    return result;
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
