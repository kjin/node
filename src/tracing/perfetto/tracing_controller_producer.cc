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
  // Hack to prevent flushes from happen at every trace event.
  // Every flush posts a new task to the task runner, so this might be needed
  // to prevent infinite loops.
  if (std::strcmp(name, "CheckImmediate") == 0) {
    return kNoHandle;
  }
  if (!this->enabled_) {
    return kNoHandle;
  }
  auto trace_writer = producer_->GetTLSTraceWriter();
  static uint64_t handle = kNoHandle + 1;
  static std::hash<std::thread::id> hasher;
  static std::mutex writer_lock_;
  std::lock_guard<std::mutex> scoped_lock(writer_lock_);
  {
    auto trace_packet = trace_writer->NewTracePacket();
    auto trace_event_bundle = trace_packet->set_chrome_events();
    auto trace_event = trace_event_bundle->add_trace_events();
    trace_event->set_process_id(uv_os_getpid());
    // Imperfect way to get a thread ID in lieu of V8's built-in mechanism.
    trace_event->set_thread_id(hasher(std::this_thread::get_id()) % 65536);
    trace_event->set_phase(phase);
    trace_event->set_category_group_name(category_manager_.GetCategoryGroupName(category_enabled_flag));
    trace_event->set_name(name);
    if (scope != nullptr) {
      trace_event->set_scope(scope);
    }
    trace_event->set_id(id);
    trace_event->set_bind_id(bind_id);
    trace_event->set_timestamp(timestamp);
    trace_event->set_thread_timestamp(timestamp);
    trace_event->set_duration(0);
    trace_event->set_thread_duration(0);
  }
  trace_writer->Flush();
  // if (!producer_->GetTaskRunner().expired()) {
  //   producer_->GetTaskRunner().lock()->PostTask([=]() {
  //     if (!trace_writer.expired()) {
  //       trace_writer.lock()->Flush();
  //     }
  //   });
  // }
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
  : trace_controller_(new PerfettoTracingController(this)) {
}

void TracingControllerProducer::OnConnect() {
  perfetto::DataSourceDescriptor ds_desc;
  ds_desc.set_name("trace_events");
  GetServiceEndpoint()->RegisterDataSource(ds_desc);
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
  {
    auto observers = trace_controller_->observers_;
    for (auto itr = observers.begin(); itr != observers.end(); itr++) {
      (*itr)->OnTraceEnabled();
    }
  }
  target_buffer_ = cfg.target_buffer();
  trace_controller_->enabled_ = true;
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

perfetto::TraceWriter* TracingControllerProducer::GetTLSTraceWriter() {
  if (!trace_writer_) {
    trace_writer_ = GetServiceEndpoint()->CreateTraceWriter(target_buffer_);
  }
  return trace_writer_.get();
}

// std::weak_ptr<perfetto::TraceWriter> TracingControllerProducer::GetTLSTraceWriter() {
//     static std::mutex writer_lock_;
//     std::lock_guard<std::mutex> scoped_lock(writer_lock_);
//     if (!svc_endpoint_ || target_buffer_ == 0) {
//       return std::weak_ptr<perfetto::TraceWriter>();
//     } else {
//       static uint8_t thread_id_ctr = 0;
//       thread_local uint8_t thread_id = thread_id_ctr++;
//       std::weak_ptr<perfetto::TraceWriter> result(writers_[thread_id]);
//       if (result.expired()) {
//         // printf("Creating new trace writer %d\n", thread_id);
//         result = std::weak_ptr<perfetto::TraceWriter>(writers_[thread_id] = svc_endpoint_->CreateTraceWriter(target_buffer_));
//       }
//       return result;
//     }
//   }

void TracingControllerProducer::Cleanup() {
  if (trace_controller_->enabled_) {
    trace_controller_->enabled_ = false;
    {
      std::list<const char*> groups = {};
      trace_controller_->category_manager_.UpdateCategoryGroups(groups);
      auto observers = trace_controller_->observers_;
      for (auto itr = observers.begin(); itr != observers.end(); itr++) {
        (*itr)->OnTraceDisabled();
      }
    }
    trace_writer_.reset(nullptr);
  }
}

}
}
