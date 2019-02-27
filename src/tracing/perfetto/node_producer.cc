#include "tracing/perfetto/node_producer.h"

namespace node {
namespace tracing {

NodeProducer::NodeProducer(): tracing_controller_(new TracingController()) {
  tracing_controller_->Initialize(nullptr);
}

void NodeProducer::Connect(perfetto::TracingService* service) {
  svc_endpoint_ = service->ConnectProducer(this, 0, "node.perfetto-producer");
}

}
}
