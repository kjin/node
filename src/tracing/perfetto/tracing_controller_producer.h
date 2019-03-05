#ifndef SRC_TRACING_PERFETTO_NODE_TRACING_CONTROLLER_PRODUCER_H_
#define SRC_TRACING_PERFETTO_NODE_TRACING_CONTROLLER_PRODUCER_H_

#include "tracing/agent.h"
#include "tracing/perfetto/node_tracing.h"
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

namespace node {
namespace tracing {

class TracingControllerProducer;

class PerfettoTracingController : public TracingController {
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
  // V8 probably does a much better job at handling categories.
  // Just do something that will probably work OK for the demo.
  class CategoryManager {
   public:
    CategoryManager(): categories_(default_categories_) {}

    const uint8_t* GetCategoryGroupEnabled(const char* group) {
      if (category_groups_.find(group) == category_groups_.end()) {
        uint8_t flags = 0x0;
        std::string clamped_group = std::string() + ',' + group + ',';
        for (auto itr = categories_.begin(); itr != categories_.end(); itr++) {
          size_t idx = clamped_group.find(std::string() + ',' + *itr + ',');
          if (idx != std::string::npos) {
            flags = 0x1;
            break;
          }
        }
        category_groups_[group] = flags;
      }
      return &category_groups_[group];
    }

    void UpdateCategoryGroups(std::list<const char*>& categories) {
      categories_ = std::list<const char*>(default_categories_);
      categories_.insert(categories_.end(), categories.begin(), categories.end());
      std::unordered_map<const char*, uint8_t> new_category_groups;
      for (auto g_itr = category_groups_.begin(); g_itr != category_groups_.end(); g_itr++) {
        uint8_t flags = 0x0;
        std::string clamped_group = std::string() + ',' + g_itr->first + ',';
        for (auto itr = categories_.begin(); itr != categories_.end(); itr++) {
          size_t idx = clamped_group.find(std::string() + ',' + *itr + ',');
          if (idx != std::string::npos) {
            flags = 0x1;
            break;
          }
        }
        new_category_groups[g_itr->first] = flags;
      }
      category_groups_ = new_category_groups;
    }
   private:
    // Maps category group to their decomposed categories
    std::unordered_map<const char*, uint8_t> category_groups_;
    const std::list<const char*> default_categories_ = { "__metadata" };
    std::list<const char*> categories_;
  } category_manager_;

  std::unique_ptr<perfetto::TraceWriter> trace_writer_;
  std::set<TraceStateObserver*> observers_;
  bool enabled_ = false;
  friend class TracingControllerProducer;
};

class TracingControllerProducer : public NodeProducer {
 public:
  TracingControllerProducer();
  std::shared_ptr<TracingController> GetTracingController() {
    return trace_controller_;
  }
  void Disconnect() override {
    Cleanup();
    NodeProducer::Disconnect();
  }
 private:
  void OnConnect() override;
  void OnDisconnect() override;

  void OnTracingSetup() override;

  void SetupDataSource(perfetto::DataSourceInstanceID id,
                       const perfetto::DataSourceConfig& cfg) override;

  void StartDataSource(perfetto::DataSourceInstanceID id,
                       const perfetto::DataSourceConfig& cfg) override;
  void StopDataSource(perfetto::DataSourceInstanceID id) override;

  void Cleanup();

  std::shared_ptr<PerfettoTracingController> trace_controller_;
  perfetto::DataSourceInstanceID data_source_id_;
};

}
}

#endif