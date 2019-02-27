#include "tracing/agent.h"
#include "trace_event.h"

namespace node {
namespace tracing {

void TracingController::AddMetadataEvent(
    const unsigned char* category_group_enabled,
    const char* name,
    int num_args,
    const char** arg_names,
    const unsigned char* arg_types,
    const uint64_t* arg_values,
    std::unique_ptr<v8::ConvertableToTraceFormat>* convertable_values,
    unsigned int flags) {
  std::unique_ptr<TraceObject> trace_event(new TraceObject);
  trace_event->Initialize(
      TRACE_EVENT_PHASE_METADATA, category_group_enabled, name,
      node::tracing::kGlobalScope,  // scope
      node::tracing::kNoId,         // id
      node::tracing::kNoId,         // bind_id
      num_args, arg_names, arg_types, arg_values, convertable_values,
      TRACE_EVENT_FLAG_NONE,
      CurrentTimestampMicroseconds(),
      CurrentCpuTimestampMicroseconds());
  node::tracing::TraceEventHelper::GetAgent()->AddMetadataEvent(
      std::move(trace_event));
}

}  // namespace tracing
}  // namespace node
