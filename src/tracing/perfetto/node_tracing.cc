#include "tracing/perfetto/node_tracing.h"
#include "tracing/perfetto/node_shared_memory.h"

namespace node {
namespace tracing {

NodeTracing::NodeTracing()
  : task_runner_(new DelayedNodeTaskRunner()) {
  tracing_service_ = perfetto::TracingService::CreateInstance(
    std::unique_ptr<perfetto::SharedMemory::Factory>(new NodeShmemFactory()),
    task_runner_.get()
  );
}

NodeTracing::~NodeTracing() {
  for (auto itr = producers_.begin(); itr != producers_.end(); itr++) {
    if (!itr->expired()) {
      std::shared_ptr<NodeProducer> producer(*itr);
      producer->Disconnect();
    }
  }
  for (auto itr = consumers_.begin(); itr != consumers_.end(); itr++) {
    if (!itr->expired()) {
      std::shared_ptr<NodeConsumer> consumer(*itr);
      consumer->Disconnect();
    }
  }
  tracing_service_.reset(nullptr);
}

void NodeTracing::Initialize() {
  task_runner_->Start();
}

std::unique_ptr<NodeConsumerHandle> NodeTracing::ConnectConsumer(std::unique_ptr<NodeConsumer> consumer) {
  consumer->Connect(tracing_service_.get());
  std::unique_ptr<NodeConsumerHandle> handle = std::unique_ptr<NodeConsumerHandle>(
    new NodeConsumerHandle(std::move(consumer), task_runner_));
  consumers_.push_back(std::weak_ptr<NodeConsumer>(handle->consumer_));
  return handle;
}

std::unique_ptr<NodeProducerHandle> NodeTracing::ConnectProducer(std::unique_ptr<NodeProducer> producer) {
  producer->Connect(tracing_service_.get());
  std::unique_ptr<NodeProducerHandle> handle = std::unique_ptr<NodeProducerHandle>(
    new NodeProducerHandle(std::move(producer), task_runner_));
  producers_.push_back(std::weak_ptr<NodeProducer>(handle->producer_));
  return handle;
}

}
}