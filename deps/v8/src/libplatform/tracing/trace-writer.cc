// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/libplatform/tracing/trace-writer.h"

#include <cmath>

#include "base/trace_event/common/trace_event_common.h"
#include "src/base/platform/platform.h"

namespace v8 {
namespace platform {
namespace tracing {

JSONTraceWriter::JSONTraceWriter(std::ostream& stream)
    : trace_serializer_(stream) {
  trace_serializer_.WritePrefix();
}

JSONTraceWriter::~JSONTraceWriter() { trace_serializer_.WriteSuffix(); }

void JSONTraceWriter::AppendTraceEvent(TraceObject* trace_event) {
  trace_serializer_.AppendTraceEvent(trace_event);
}

void JSONTraceWriter::Flush() {}

TraceWriter* TraceWriter::CreateJSONTraceWriter(std::ostream& stream) {
  return new JSONTraceWriter(stream);
}

}  // namespace tracing
}  // namespace platform
}  // namespace v8
