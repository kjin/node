#ifndef SRC_NODE_TRACING_H_
#define SRC_NODE_TRACING_H_

#include "tracing/base/node_tracing.h"

namespace node {

namespace per_process {
  tracing::base::NodeTracing tracing;
}

inline tracing::base::NodeTracing& GetNodeTracing() {
  return per_process::tracing;
}

}

#endif
