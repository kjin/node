#include "tracing/perfetto/tracing_controller_producer.h"
#include "stdio.h"
#include "perfetto/trace/clock_snapshot.pbzero.h"

namespace node {
namespace tracing {

const uint64_t kNoHandle = 0;

uint64_t PerfettoTracingController::AddTraceEventWithTimestamp(
    char phase, const uint8_t* category_enabled_flag, const char* name,
    const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
    const char** arg_names, const uint8_t* arg_types,
    const uint64_t* arg_values,
    std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
    unsigned int flags, int64_t timestamp) {
  if (!trace_writer_) {
    return kNoHandle;
  }
  static uint64_t handle = kNoHandle + 1;
  {
    auto trace_packet = trace_writer_->NewTracePacket();
    auto trace_event_bundle = trace_packet->set_chrome_events();
    auto trace_event = trace_event_bundle->add_trace_events();
    // This is taken from the V8 prototype.
    trace_event->set_phase(phase == 'X' ? 'B' : phase);
    trace_event->set_name(name);
    if (scope != nullptr) {
      trace_event->set_scope(scope);
    }
    trace_event->set_id(id);
    trace_event->set_bind_id(bind_id);
    trace_event->set_timestamp(timestamp);
    trace_event->set_thread_timestamp(timestamp);
  }
  trace_writer_->Flush();
  printf("Trace event added: %s (bytes written: %llu)\n", name, trace_writer_->written());
  return handle++;
}

TracingControllerNodeProducer::TracingControllerNodeProducer()
  : trace_controller_(new PerfettoTracingController()) {
  name_ = "node";
}

void TracingControllerNodeProducer::OnConnect() {
  printf("Connected\n");
  perfetto::DataSourceDescriptor ds_desc;
  ds_desc.set_name("trace_events");
  svc_endpoint_->RegisterDataSource(ds_desc);
}

void TracingControllerNodeProducer::OnDisconnect() {
  Cleanup();
}

void TracingControllerNodeProducer::OnTracingSetup() {
  printf("Tracing set up\n");
}

void TracingControllerNodeProducer::SetupDataSource(perfetto::DataSourceInstanceID id,
                      const perfetto::DataSourceConfig& cfg) {
  data_source_id_ = id;
}

void TracingControllerNodeProducer::StartDataSource(perfetto::DataSourceInstanceID id,
                      const perfetto::DataSourceConfig& cfg) {
  if (data_source_id_ != id) {
    return;
  }
  trace_controller_->trace_writer_ = svc_endpoint_->CreateTraceWriter(cfg.target_buffer());
  // TODO(kjin): Don't hardcode these
  trace_controller_->category_groups_["node"] = 255;
  trace_controller_->category_groups_["node.async_hooks"] = 255;
  trace_controller_->category_groups_["v8"] = 255;
}

void TracingControllerNodeProducer::StopDataSource(perfetto::DataSourceInstanceID id) {
  if (data_source_id_ != id) {
    return;
  }
  Cleanup();
}

void TracingControllerNodeProducer::Cleanup() {
  trace_controller_->trace_writer_.reset(nullptr);
  trace_controller_->category_groups_.clear();
}

}
}
