#ifndef SRC_TRACING_PERFETTO_NODE_CONSUMER_H_
#define SRC_TRACING_PERFETTO_NODE_CONSUMER_H_

#include "perfetto/tracing/core/consumer.h"
#include "perfetto/tracing/core/tracing_service.h"
#include "perfetto/tracing/core/trace_config.h"
#include "tracing/agent.h"
#include "node_task_runner.h"

namespace node {
namespace tracing {

class PerfettoConsumerHandle;

class NodeConsumer : public perfetto::Consumer {
 public:
  void Connect(perfetto::TracingService* service);
  void Disconnect();
 protected:
  std::unique_ptr<perfetto::TracingService::ConsumerEndpoint> svc_endpoint_;
  perfetto::TraceConfig trace_config_;
  bool is_default_ = false;
  //
  void OnConnect() override = 0;
  void OnDisconnect() override = 0;
  void OnTracingDisabled() override = 0;
  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override = 0;
  void OnDetach(bool success) override = 0;
  void OnAttach(bool success, const perfetto::TraceConfig&) override = 0;

  void OnTraceStats(bool success, const perfetto::TraceStats&) override = 0;

  friend class PerfettoConsumerHandle;
};

class DefaultNodeConsumer : public NodeConsumer {
 public:
  DefaultNodeConsumer();
  ~DefaultNodeConsumer() {
    svc_endpoint_.reset();
  }
 private:
  void OnConnect() override { PERFETTO_DLOG("Node consumer connected"); }
  void OnDisconnect() override { PERFETTO_DLOG("Node consumer disconnected"); }
  void OnTracingDisabled() override {
    PERFETTO_DLOG("Node consumer trace disabled");
  }

  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override {
    // PERFETTO_DLOG("Node consumer packets %zu %d", packets.size(), has_more);
  }

  void OnDetach(bool success) override {
    PERFETTO_DLOG("Node consumer OnDetach");
  }

  void OnAttach(bool success, const perfetto::TraceConfig&) override {
    PERFETTO_DLOG("Node consumer OnAttach");
  }

  void OnTraceStats(bool success, const perfetto::TraceStats&) override {
    PERFETTO_DLOG("Node consumer OnTraceStats");
  }
};

/**
 * A wrapper around AsyncTraceWriter. Not necessarily efficient.
 */
class AsyncTraceWriterConsumer : public NodeConsumer {
 public:
  AsyncTraceWriterConsumer(std::unique_ptr<AsyncTraceWriter> writer): writer_(std::move(writer)) {}
  ~AsyncTraceWriterConsumer() {
    Disconnect();
  }
 private:
  void OnConnect() override { PERFETTO_DLOG("Node consumer connected"); }
  void OnDisconnect() override { PERFETTO_DLOG("Node consumer disconnected"); }
  void OnTracingDisabled() override {
    PERFETTO_DLOG("Node consumer trace disabled");
  }

  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override {
    // PERFETTO_DLOG("Node consumer packets %zu %d", packets.size(), has_more);
  }

  void OnDetach(bool success) override {
    PERFETTO_DLOG("Node consumer OnDetach");
  }

  void OnAttach(bool success, const perfetto::TraceConfig&) override {
    PERFETTO_DLOG("Node consumer OnAttach");
  }

  void OnTraceStats(bool success, const perfetto::TraceStats&) override {
    PERFETTO_DLOG("Node consumer OnTraceStats");
  }

  std::unique_ptr<AsyncTraceWriter> writer_;
};

class PerfettoConsumerHandle : public AgentWriterHandle {
 public:
  inline PerfettoConsumerHandle(
    std::shared_ptr<NodeConsumer> consumer,
    Agent* agent,
    NodeTaskRunner* task_runner) : consumer_(std::move(consumer)), agent_(agent), task_runner_(task_runner) {}
  ~PerfettoConsumerHandle() {
    if (consumer_->svc_endpoint_) { // Or, consumer is already disconnected
      consumer_->Disconnect();
      // Don't destroy the Consumer right away.
      std::shared_ptr<NodeConsumer> consumer = this->consumer_;
      task_runner_->PostTask([consumer]() {});
    }
  }

  // TODO(kjin): Needs assignment overloads.

  inline bool empty() const override { return agent_ == nullptr; }
  inline void reset() override {}

  void Enable(const std::set<std::string>& categories) override;
  void Disable(const std::set<std::string>& categories) override;

  inline bool IsDefaultHandle() override { return consumer_->is_default_; }

  inline Agent* agent() override { return agent_; }

  inline v8::TracingController* GetTracingController() override {
    return agent_->GetTracingController();
  }
 private:
  std::shared_ptr<NodeConsumer> consumer_;
  Agent* agent_;
  NodeTaskRunner* task_runner_;
};

}
}

#endif