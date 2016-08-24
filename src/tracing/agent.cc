#include "tracing/agent.h"

#include "env-inl.h"
#include "libplatform/libplatform.h"
#include "tracing/trace_config_parser.h"

namespace node {
namespace tracing {

Agent::Agent(Environment* env) : parent_env_(env) {}

void Agent::Start(v8::Platform* platform, const char* trace_config_file) {
  auto env = parent_env_;
  platform_ = platform;

  NodeTraceWriter* trace_writer = new NodeTraceWriter();
  TraceBuffer* trace_buffer = new NodeTraceBuffer(
      NodeTraceBuffer::kBufferChunks, trace_writer);

  tracing_controller_ = new TracingController();

  TraceConfig* trace_config = new TraceConfig();
  if (trace_config_file) {
    std::ifstream fin(trace_config_file);
    std::string str((std::istreambuf_iterator<char>(fin)),
                    std::istreambuf_iterator<char>());
    TraceConfigParser::FillTraceConfig(env->isolate(), trace_config,
                                       str.c_str());
  } else {
     trace_config->AddIncludedCategory("v8");
     trace_config->AddIncludedCategory("node");
  }
  tracing_controller_->Initialize(trace_buffer);
  tracing_controller_->StartTracing(trace_config);
  v8::platform::SetTracingController(platform, tracing_controller_);
}

void Agent::Stop() {
  if (!IsStarted()) {
    return;
  }
  // Perform final Flush on TraceBuffer. We don't want the tracing controller
  // to flush the buffer again on destruction of the V8::Platform.
  tracing_controller_->StopTracing();
  delete tracing_controller_;
  v8::platform::SetTracingController(platform_, nullptr);
}

}  // namespace tracing
}  // namespace node
