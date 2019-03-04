#include "tracing/perfetto/node_task_runner.h"
#include "node_v8_platform-inl.h"
#include "debug_utils.h"
#include <stdio.h>

namespace node {
namespace tracing {

class TracingTask : public v8::Task {
 public:
  explicit TracingTask(std::function<void()> f) : f_(std::move(f)) {
    // TODO(kjin): For debugging purposes only, remove later.
    static uint32_t next_task_id_ = 0;
    task_id_ = next_task_id_++;
    // printf("Initialized task %d\n", task_id_);
  }

  void Run() override {
    // printf("Running task %d\n", task_id_);
    f_();
  }

 private:
  std::function<void()> f_;
  uint32_t task_id_;
};

void NodeTaskRunner::PostTask(std::function<void()> task) {
  NodePlatform* platform = per_process::v8_platform.Platform();
  if (platform == nullptr) {
    return; // No platform -- process is going down soon anyway.
  }
  platform->CallOnForegroundThread(v8::Isolate::GetCurrent(), new TracingTask(std::move(task)));
}

void NodeTaskRunner::PostDelayedTask(std::function<void()> task, uint32_t delay_ms) {
  NodePlatform* platform = per_process::v8_platform.Platform();
  if (platform == nullptr) {
    return; // No platform -- process is going down soon anyway.
  }
  platform->CallDelayedOnForegroundThread(v8::Isolate::GetCurrent(), new TracingTask(std::move(task)), delay_ms / 1000.0);
}

void DelayedNodeTaskRunner::PostTask(std::function<void()> task) {
  if (!started_) {
    delayed_args_.push_back(std::pair<std::function<void()>, uint32_t>(std::move(task), 0));
  } else {
    NodeTaskRunner::PostTask(task);
  }
}

void DelayedNodeTaskRunner::PostDelayedTask(std::function<void()> task, uint32_t delay_ms) {
  if (!started_) {
    delayed_args_.push_back(std::pair<std::function<void()>, uint32_t>(std::move(task), delay_ms));
  } else {
    NodeTaskRunner::PostDelayedTask(task, delay_ms);
  }
}

void DelayedNodeTaskRunner::Start() {
  started_ = true;
  for (auto itr = delayed_args_.begin(); itr != delayed_args_.end(); itr++) {
    if (itr->second == 0) {
      PostTask(itr->first);
    } else {
      PostDelayedTask(itr->first, itr->second);
    }
  }
  delayed_args_.clear();
}

}
}
