#ifndef SRC_TRACING_AGENT_H_
#define SRC_TRACING_AGENT_H_

#include "libplatform/v8-tracing.h"
#include "uv.h"
#include "v8.h"
#include "util.h"
#include "node_mutex.h"

#include <set>
#include <string>

namespace node {
namespace tracing {

using v8::platform::tracing::TraceConfig;
using v8::platform::tracing::TraceObject;
using v8::platform::tracing::TraceBuffer;

class Agent;

class AsyncTraceWriter {
 public:
  virtual ~AsyncTraceWriter() {}
  virtual void AppendTraceEvent(TraceObject* trace_event) = 0;
  virtual void Flush(bool blocking) = 0;
  virtual void InitializeOnThread(uv_loop_t* loop) {}
};

class TracingController : public v8::TracingController {
 public:
  virtual void Initialize(TraceBuffer* trace_buffer) = 0;
  virtual void AddMetadataEvent(
      const unsigned char* category_group_enabled,
      const char* name,
      int num_args,
      const char** arg_names,
      const unsigned char* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* convertable_values,
      unsigned int flags) = 0;
  virtual void StartTracing(TraceConfig* trace_config) = 0;
  virtual void StopTracing() = 0;
};

enum UseDefaultCategoryMode {
  kUseDefaultCategories,
  kIgnoreDefaultCategories
};

class AgentWriterHandle {
 public:
  virtual ~AgentWriterHandle() {}
  virtual bool empty() const = 0;
  virtual void reset() = 0;

  virtual void Enable(const std::set<std::string>& categories) = 0;
  virtual void Disable(const std::set<std::string>& categories) = 0;

  virtual bool IsDefaultHandle() = 0;

  virtual Agent* agent() = 0;

  virtual v8::TracingController* GetTracingController() = 0;
};

class Agent {
 public:
  virtual ~Agent() {}
  virtual void Initialize() {}
  virtual TracingController* GetTracingController() = 0;
  virtual std::unique_ptr<AgentWriterHandle> AddClient(const std::set<std::string>& categories,
                              std::unique_ptr<AsyncTraceWriter> writer,
                              enum UseDefaultCategoryMode mode) = 0;
  virtual std::unique_ptr<AgentWriterHandle> DefaultHandle() = 0;
  virtual std::string GetEnabledCategories() const = 0;
  virtual void AddMetadataEvent(std::unique_ptr<TraceObject> event) = 0;
};

}  // namespace tracing
}  // namespace node

#endif  // SRC_TRACING_AGENT_H_
