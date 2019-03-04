#include "tracing/perfetto/file_writer_consumer.h"
#include "perfetto/base/scoped_file.h"
#include "perfetto/tracing/core/trace_config.h"

namespace node {
namespace tracing {

void FileWriterConsumer::Enable(const std::set<std::string>& categories) {
  if (task_runner_.expired()) {
    // Not connected yet, so can't enable.
    UNREACHABLE();
  }
  task_runner_.lock()->PostTask([=]() {
    if (!connected_) {
      return;
    }
    perfetto::TraceConfig config;
    config.add_buffers()->set_size_kb(options_.buffer_size_kb);
    {
      auto c = config.add_data_sources();
      auto c2 = c->mutable_config();
      c2->set_name("trace_events");
    }
    config.set_write_into_file(true);
    config.set_disable_clock_snapshotting(true);
    config.set_file_write_period_ms(options_.file_write_period_ms);
    
    perfetto::base::ScopedFile fd(
        open(options_.filename, O_RDWR | O_CREAT | O_TRUNC, 0644));
    svc_endpoint_->EnableTracing(config, std::move(fd));
  });
}

void FileWriterConsumer::Disable(const std::set<std::string>& categories) {
  if (task_runner_.expired()) {
    // Not connected yet, no point in disabling.
    UNREACHABLE();
  }
  task_runner_.lock()->PostTask([=]() {
    if (connected_) {
      svc_endpoint_->DisableTracing();
    }
  });
}

}
}