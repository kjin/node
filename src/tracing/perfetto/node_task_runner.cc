#include "tracing/perfetto/node_task_runner.h"
#include "node_v8_platform-inl.h"

namespace node {
namespace tracing {

class TracingTask : public v8::Task {
 public:
  explicit TracingTask(std::function<void()> f) : f_(std::move(f)) {}

  void Run() override { f_(); }

 private:
  std::function<void()> f_;
};

void NodeTaskRunner::PostTask(std::function<void()> task) {
  if (isolate_ == nullptr) {
    return;
  }
  per_process::v8_platform.Platform()->CallOnWorkerThread(std::unique_ptr<TracingTask>(new TracingTask(std::move(task))));
}

void NodeTaskRunner::PostDelayedTask(std::function<void()> task, uint32_t delay_ms) {
  if (isolate_ == nullptr) {
    return;
  }
  per_process::v8_platform.Platform()->CallDelayedOnWorkerThread(std::unique_ptr<TracingTask>(new TracingTask(std::move(task))), delay_ms * 1000.0);
}

}
}
