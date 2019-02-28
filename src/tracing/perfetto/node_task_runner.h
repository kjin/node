#ifndef SRC_TRACING_PERFETTO_NODE_TASK_RUNNER_H_
#define SRC_TRACING_PERFETTO_NODE_TASK_RUNNER_H_

#include "perfetto/base/task_runner.h"
#include "debug_utils.h"
#include "uv.h"
#include "v8.h"

namespace node {
namespace tracing {

class NodeTaskRunner : public perfetto::base::TaskRunner {
 public:
  NodeTaskRunner() {}
  ~NodeTaskRunner() {}

  void PostTask(std::function<void()> task) override;

  void PostDelayedTask(std::function<void()> task, uint32_t delay_ms) override;

  void AddFileDescriptorWatch(int fd, std::function<void()>) override {}
  void RemoveFileDescriptorWatch(int fd) override {}
  bool RunsTasksOnCurrentThread() const override { return false; }
 private:
  v8::Isolate* isolate_ = nullptr;
};

}
}

#endif
