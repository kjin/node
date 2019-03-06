#ifndef SRC_TRACING_PERFETTO_NODE_TRACING_CONTROLLER_PRODUCER_H_
#define SRC_TRACING_PERFETTO_NODE_TRACING_CONTROLLER_PRODUCER_H_

#include "libplatform/v8-tracing.h"
#include "tracing/perfetto/node_tracing.h"
#include "tracing/agent.h"
#include "uv.h"
#include "perfetto/tracing/core/data_source_descriptor.h"
#include "perfetto/tracing/core/trace_config.h"
#include "perfetto/trace/chrome/chrome_trace_event.pbzero.h"
#include "perfetto/trace/trace_packet.pbzero.h"
#include "perfetto/tracing/core/trace_writer.h"
#include "perfetto/protozero/message_handle.h"
#include "perfetto/tracing/core/basic_types.h"
#include "perfetto/protozero/message.h"
#include <set>
#include <mutex>

using V8TracingController = v8::platform::tracing::TracingController;
using NodeTracingController = node::tracing::TracingController;

namespace node {
namespace tracing {

class TracingControllerProducer;

class PerfettoTracingController : public NodeTracingController {
 public:
  // node::tracing::TracingController -- should all be no-ops except metadata
  void Initialize(TraceBuffer* trace_buffer) override {
    // Do nothing
  }
  void AddMetadataEvent(
      const unsigned char* category_group_enabled,
      const char* name,
      int num_args,
      const char** arg_names,
      const unsigned char* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* convertable_values,
      unsigned int flags) override {
        // Do nothing
      }
  void StartTracing(TraceConfig* trace_config) override {}
  void StopTracing() override {}

  // v8::TracingController
  const uint8_t* GetCategoryGroupEnabled(const char* name) override {
    return category_manager_.GetCategoryGroupEnabled(name);
  }

  uint64_t AddTraceEvent(
      char phase, const uint8_t* category_enabled_flag, const char* name,
      const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
      const char** arg_names, const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags) override {
    int64_t timestamp = uv_hrtime() / 1000;
    return AddTraceEventWithTimestamp(
      phase, category_enabled_flag, name, scope, id, bind_id, num_args,
      arg_names, arg_types, arg_values, std::move(arg_convertables), flags,
      timestamp);
  }
  uint64_t AddTraceEventWithTimestamp(
      char phase, const uint8_t* category_enabled_flag, const char* name,
      const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
      const char** arg_names, const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags, int64_t timestamp) override;
  void UpdateTraceEventDuration(const uint8_t* category_enabled_flag,
                                        const char* name, uint64_t handle) override {}
  void AddTraceStateObserver(TraceStateObserver*) override;
  void RemoveTraceStateObserver(TraceStateObserver*) override;
 private:
  PerfettoTracingController(TracingControllerProducer* producer): producer_(producer) {}

  // V8 probably does a much better job at handling categories.
  // Just do something that will probably work OK for the demo.
  class CategoryManager {
   public:
    CategoryManager() {
      controller_.Initialize(nullptr);
    }

    const uint8_t* GetCategoryGroupEnabled(const char* category_group) {
      return controller_.GetCategoryGroupEnabled(category_group);
    }

    const char* GetCategoryGroupName(const uint8_t* category_group_flags) {
      return controller_.GetCategoryGroupName(category_group_flags);
    }

    void UpdateCategoryGroups(std::list<const char*> categories) {
      auto config = v8::platform::tracing::TraceConfig::CreateDefaultTraceConfig();
      for (auto itr = categories.begin(); itr != categories.end(); itr++) {
        config->AddIncludedCategory(*itr);
      };
      controller_.StartTracing(config);
    }
   private:
    V8TracingController controller_;
  } category_manager_;

  TracingControllerProducer* producer_;
  std::set<TraceStateObserver*> observers_;
  bool enabled_ = false;
  friend class TracingControllerProducer;
};

class TracingControllerProducer : public NodeProducer {
 public:
  TracingControllerProducer();
  std::shared_ptr<NodeTracingController> GetTracingController() {
    return trace_controller_;
  }
  std::weak_ptr<perfetto::TraceWriter> GetTLSTraceWriter() {
    static std::mutex writer_lock_;
    std::lock_guard<std::mutex> scoped_lock(writer_lock_);
    if (!svc_endpoint_ || target_buffer_ == 0) {
      return std::weak_ptr<perfetto::TraceWriter>();
    } else {
      static uint8_t thread_id_ctr = 0;
      thread_local uint8_t thread_id = thread_id_ctr++;
      std::weak_ptr<perfetto::TraceWriter> result(writers_[thread_id]);
      if (result.expired()) {
        printf("Creating new trace writer %d\n", thread_id);
        result = std::weak_ptr<perfetto::TraceWriter>(writers_[thread_id] = svc_endpoint_->CreateTraceWriter(target_buffer_));
      }
      return result;
    }
  }
  std::weak_ptr<perfetto::base::TaskRunner> GetTaskRunner() {
    return task_runner_;
  }
 private:
  void BeforeDisconnect() override {
    Cleanup();
  }
  void OnConnect() override;
  void SetupDataSource(perfetto::DataSourceInstanceID id,
                       const perfetto::DataSourceConfig& cfg) override;

  void StartDataSource(perfetto::DataSourceInstanceID id,
                       const perfetto::DataSourceConfig& cfg) override;
  void StopDataSource(perfetto::DataSourceInstanceID id) override;

  void Cleanup();

  uint32_t target_buffer_ = 0;
  std::unordered_map<uint8_t, std::shared_ptr<perfetto::TraceWriter>> writers_;
  std::shared_ptr<PerfettoTracingController> trace_controller_;
  perfetto::DataSourceInstanceID data_source_id_;
};

}
}

#endif