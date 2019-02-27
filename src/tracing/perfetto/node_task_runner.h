#ifndef SRC_TRACING_PERFETTO_NODE_TASK_RUNNER_H_
#define SRC_TRACING_PERFETTO_NODE_TASK_RUNNER_H_

#include "perfetto/base/task_runner.h"
#include "debug_utils.h"
#include "uv.h"

namespace node {
namespace tracing {

/**
 * An implementation of TaskRunner on the libuv thread pool.
 */
class NodeTaskRunner : public perfetto::base::TaskRunner {
 public:
  NodeTaskRunner() {
    CHECK_EQ(uv_loop_init(&tracing_loop_), 0);
  }

  ~NodeTaskRunner() {
    Stop();
    // TODO(kjin): Verify that this is for the sake of draining the queue...
    uv_run(&tracing_loop_, UV_RUN_ONCE);
    CheckedUvLoopClose(&tracing_loop_);
  }

  void PostTask(std::function<void()> task) override {
    uv_async_t async;
    CHECK_EQ(uv_async_init(&tracing_loop_, &async, [](uv_async_t* async) {
      std::function<void()> task = *static_cast<std::function<void()>*>(async->data);
      task();
    }), 0);
    // This probably won't work.
    async.data = &task;
    CHECK_EQ(uv_async_send(&async), 0);
  }

  void PostDelayedTask(std::function<void()> task, uint32_t delay_ms) override {
    uv_timer_t timer;
    CHECK_EQ(uv_timer_init(&tracing_loop_, &timer), 0);
    // This probably won't work.
    timer.data = &task;
    CHECK_EQ(uv_timer_start(&timer, [](uv_timer_t* timer) {
      std::function<void()> task = *static_cast<std::function<void()>*>(timer->data);
      task();
    }, delay_ms, 0), 0);
  }
  void AddFileDescriptorWatch(int fd, std::function<void()>) override {}
  void RemoveFileDescriptorWatch(int fd) override {}
  bool RunsTasksOnCurrentThread() const override { return false; }

  void Start() {
    uv_thread_create(&thread_, [](void* arg) {
      NodeTaskRunner* agent = static_cast<NodeTaskRunner*>(arg);
      uv_run(&agent->tracing_loop_, UV_RUN_DEFAULT);
    }, this);
  }

  void Stop() {
    uv_thread_join(&thread_);
  }
 private:
  uv_thread_t thread_;
  uv_loop_t tracing_loop_;
};

}
}

#endif
