#include "tracing/perfetto_agent.h"
#include "tracing/perfetto/node_shared_memory.h"
#include "tracing/perfetto/tracing_controller.h"
#include "perfetto/tracing/core/data_source_descriptor.h"
#include "perfetto/tracing/core/trace_config.h"
#include "perfetto/tracing/core/trace_packet.h"
#include "perfetto/trace/chrome/chrome_trace_event.pbzero.h"
#include "perfetto/trace/trace_packet.pbzero.h"
#include "perfetto/tracing/core/trace_writer.h"
#include "perfetto/protozero/message_handle.h"
#include "perfetto/tracing/core/basic_types.h"
#include "perfetto/protozero/message.h"

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
    // {
    //   auto c = config.add_producers();
    //   c->set_producer_name("node");
    //   c->set_shm_size_kb(4096);
    //   c->set_page_size_kb(4096);
    // }
    config.set_duration_ms(1000);
    svc_endpoint_->EnableTracing(config);
  }
};

class TraceControllerNodeProducer : public NodeProducer {
 public:
  TraceControllerNodeProducer(TracingController* trace_controller): trace_controller_(trace_controller) {
    name_ = "node";
  }
 private:
  class PerfettoTraceBuffer : public v8::platform::tracing::TraceBuffer {
   public:
    PerfettoTraceBuffer(std::unique_ptr<perfetto::TraceWriter> writer): writer_(std::move(writer)) {}
    ~PerfettoTraceBuffer() {}
    TraceObject* AddTraceEvent(uint64_t* handle) override {
      auto obj = new PerfettoTraceObject(writer_->NewTracePacket());
      return obj;
    }
    TraceObject* GetEventByHandle(uint64_t handle) override {
      return nullptr;
    }
    bool Flush() override {
      printf("Flushing\n");
      writer_->Flush();
      return true;
    }
   private:
    class PerfettoTraceObject : public v8::platform::tracing::TraceObject {
     public:
      PerfettoTraceObject(perfetto::TraceWriter::TracePacketHandle&& handle)
        : handle_(handle) {}
      ~PerfettoTraceObject() {
        printf("Writing an object\n");
        auto trace_event = handle_->set_chrome_events()->add_trace_events();
        trace_event->set_phase(phase() == 'X' ? 'B' : phase());
        trace_event->set_category_group_name("TODO");
        trace_event->set_name(name());
        trace_event->set_timestamp(duration());
        trace_event->set_thread_timestamp(cpu_duration());
      }
     private:
      perfetto::TraceWriter::TracePacketHandle& handle_;
    };
    std::unique_ptr<perfetto::TraceWriter> writer_;
  };

  void OnConnect() override {
    printf("Connected\n");
    perfetto::DataSourceDescriptor ds_desc;
    ds_desc.set_name("trace_events");
    svc_endpoint_->RegisterDataSource(ds_desc);
  }

  void OnDisconnect() override {
    printf("Disconnected\n");
  }

  void OnTracingSetup() override {
    printf("Tracing set up\n");
  }

  void SetupDataSource(perfetto::DataSourceInstanceID id,
                       const perfetto::DataSourceConfig& cfg) override {
    printf("Data source set up %llu\n", id);
  }

  void StartDataSource(perfetto::DataSourceInstanceID id,
                       const perfetto::DataSourceConfig& cfg) override {
    printf("Data source started %llu\n", id);
    auto writer = svc_endpoint_->CreateTraceWriter(cfg.target_buffer());
    trace_controller_->Initialize(new PerfettoTraceBuffer(std::move(writer)));
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
  : node_tracing_(new NodeTracing()), tracing_controller_(new PerfettoTracingController()) {
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
