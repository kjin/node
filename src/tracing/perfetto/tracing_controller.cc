#include "tracing/perfetto/tracing_controller.h"
#include "stdio.h"

namespace node {
namespace tracing {

void PerfettoTracingController::StartTracing(TraceConfig* trace_config) {
}

void PerfettoTracingController::StopTracing() {
}

uint64_t PerfettoTracingController::AddTraceEventWithTimestamp(
    char phase, const uint8_t* category_enabled_flag, const char* name,
    const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
    const char** arg_names, const uint8_t* arg_types,
    const uint64_t* arg_values,
    std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
    unsigned int flags, int64_t timestamp) {
  printf("Trace event added.\n");
}

}
}
