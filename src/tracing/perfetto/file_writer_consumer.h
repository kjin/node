#ifndef SRC_TRACING_PERFETTO_FILE_WRITER_CONSUMER_H_
#define SRC_TRACING_PERFETTO_FILE_WRITER_CONSUMER_H_

#include "tracing/perfetto_agent.h"

namespace node {
namespace tracing {

struct WriteToFileOptions {
  const char* filename;
  uint32_t buffer_size_kb;
  uint32_t file_write_period_ms;
};

class FileWriterNodeConsumer : public TracingAgentClientConsumer {
 public:
  FileWriterNodeConsumer(const WriteToFileOptions& options): options_(options) {}
  void Enable(const std::set<std::string>& categories) override;
  void Disable(const std::set<std::string>& categories) override;
 private:
  void OnConnect() override {
    connected_ = true;
  }
  void OnDisconnect() override {
    connected_ = false;
  }
  const WriteToFileOptions& options_;
  bool connected_ = false;
};

}
}

#endif