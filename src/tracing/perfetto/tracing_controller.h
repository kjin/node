#ifndef SRC_TRACING_PERFETTO_TRACING_CONTROLLER_H_
#define SRC_TRACING_PERFETTO_TRACING_CONTROLLER_H_

#include "tracing/agent.h"
#include "uv.h"

namespace node {
namespace tracing {

class PerfettoTracingController : public TracingController {
 public:
  // node::tracing::TracingController
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
  void StartTracing(TraceConfig* trace_config) override;
  void StopTracing() override;

  // v8::TracingController
  const uint8_t* GetCategoryGroupEnabled(const char* name) override {
    static uint8_t no = 0;
    return &no;
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
      phase,
      category_enabled_flag,
      name,
      scope,
      id,
      bind_id,
      num_args,
      arg_names,
      arg_types,
      arg_values,
      std::move(arg_convertables),
      flags,
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
  void AddTraceStateObserver(TraceStateObserver*) override {}
  void RemoveTraceStateObserver(TraceStateObserver*) override {}
};

}
}

#endif