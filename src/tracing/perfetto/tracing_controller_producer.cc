#include "tracing/perfetto/tracing_controller_producer.h"
#include "perfetto/trace/clock_snapshot.pbzero.h"
#include "uv.h"
#include <algorithm>
#include <thread>

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
  static std::hash<std::thread::id> hasher;
  {
    auto trace_packet = trace_writer_->NewTracePacket();
    auto trace_event_bundle = trace_packet->set_chrome_events();
    auto trace_event = trace_event_bundle->add_trace_events();
    trace_event->set_process_id(uv_os_getpid());
    // Imperfect way to get a thread ID in lieu of V8's built-in mechanism.
    trace_event->set_thread_id(hasher(std::this_thread::get_id()) % 65536);
    trace_event->set_phase(phase);
    trace_event->set_name(name);
    if (scope != nullptr) {
      trace_event->set_scope(scope);
    }
    trace_event->set_id(id);
    trace_event->set_bind_id(bind_id);
    trace_event->set_timestamp(timestamp);
    trace_event->set_thread_timestamp(timestamp);
    // trace_event->set_category_group_name(
  }
  trace_writer_->Flush();
  return handle++;
}

void PerfettoTracingController::AddTraceStateObserver(TraceStateObserver* observer) {
  if (observers_.find(observer) != observers_.end()) {
    return;
  }
  observers_.insert(observer);
  if (enabled_) {
    observer->OnTraceEnabled();
  }
}

void PerfettoTracingController::RemoveTraceStateObserver(TraceStateObserver* observer) {
  observers_.erase(observer);
}

TracingControllerProducer::TracingControllerProducer()
  : trace_controller_(new PerfettoTracingController()) {
  name_ = "node";
}

void TracingControllerProducer::OnConnect() {
  NodeProducer::OnConnect();
  printf("Connected\n");
  perfetto::DataSourceDescriptor ds_desc;
  ds_desc.set_name("trace_events");
  svc_endpoint_->RegisterDataSource(ds_desc);
}

void TracingControllerProducer::OnDisconnect() {
  NodeProducer::OnDisconnect();
  Cleanup();
}

void TracingControllerProducer::OnTracingSetup() {
  printf("Tracing set up\n");
}

void TracingControllerProducer::SetupDataSource(perfetto::DataSourceInstanceID id,
                      const perfetto::DataSourceConfig& cfg) {
  data_source_id_ = id;
}

void TracingControllerProducer::StartDataSource(perfetto::DataSourceInstanceID id,
                      const perfetto::DataSourceConfig& cfg) {
  if (data_source_id_ != id) {
    return;
  }
  trace_controller_->enabled_ = true;
  {
    auto observers = trace_controller_->observers_;
    for (auto itr = observers.begin(); itr != observers.end(); itr++) {
      (*itr)->OnTraceEnabled();
    }
  }
  trace_controller_->trace_writer_ = svc_endpoint_->CreateTraceWriter(cfg.target_buffer());
  // TODO(kjin): Don't hardcode these
  std::list<const char*> groups = { "node", "node.async_hooks", "v8" };
  trace_controller_->category_manager_.UpdateCategoryGroups(groups);
}

void TracingControllerProducer::StopDataSource(perfetto::DataSourceInstanceID id) {
  if (data_source_id_ != id) {
    return;
  }
  Cleanup();
}

void TracingControllerProducer::Cleanup() {
  trace_controller_->enabled_ = false;
  {
    std::list<const char*> groups = {};
    trace_controller_->category_manager_.UpdateCategoryGroups(groups);
    auto observers = trace_controller_->observers_;
    for (auto itr = observers.begin(); itr != observers.end(); itr++) {
      (*itr)->OnTraceDisabled();
    }
  }
  trace_controller_->trace_writer_.reset(nullptr);
  // trace_controller_->category_groups_.clear();
}

}
}
