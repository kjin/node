#include "tracing/trace_event.h"

namespace node {
namespace tracing {

Agent* g_agent = nullptr;

void TraceEventHelper::SetAgent(Agent* agent) {
  g_agent = agent;
}

Agent* TraceEventHelper::GetAgent() {
  return g_agent;
}

v8::TracingController* TraceEventHelper::GetTracingController() {
  return g_agent->GetTracingController();
}

}  // namespace tracing
}  // namespace node
