#ifndef SRC_TRACING_PERFETTO_NODE_TASK_RUNNER_H_
#define SRC_TRACING_PERFETTO_NODE_TASK_RUNNER_H_

#include "perfetto/base/task_runner.h"
#include "debug_utils.h"
#include "uv.h"
#include "v8.h"

#include <list>

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
  virtual void Start() = 0;
};

class DelayedNodeTaskRunner : public NodeTaskRunner {
 public:
  void PostTask(std::function<void()> task) override;
  void PostDelayedTask(std::function<void()> task, uint32_t delay_ms) override;
  void Start() override;
 private:
  std::list<std::pair<std::function<void()>, uint32_t>> delayed_args_;
  bool started_ = false;
};

}
}

#endif
