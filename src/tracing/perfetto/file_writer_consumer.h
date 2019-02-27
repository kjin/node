#ifndef SRC_TRACING_PERFETTO_FILE_WRITER_CONSUMER_H_
#define SRC_TRACING_PERFETTO_FILE_WRITER_CONSUMER_H_

#include "tracing/perfetto_agent.h"

namespace node {
namespace tracing {

struct FileWriterConsumerOptions {
  const char* log_file_pattern;
  // Note that this needs to be a multiple of the page size, 4kb.
  uint32_t buffer_size_kb;
  uint32_t file_write_period_ms;
  uint32_t file_size_kb;
};

/**
 * A Perfetto Consumer that doesn't consume traces directly, instead directing
 * producers to write to a file.
 */
class FileWriterConsumer : public TracingAgentClientConsumer {
 public:
  FileWriterConsumer(FileWriterConsumerOptions& options): options_(options) {}
  void Enable(const std::set<std::string>& categories) override;
  void Disable(const std::set<std::string>& categories) override;
 private:
  void OnConnect() override {
    connected_ = true;
  }
  void OnDisconnect() override {
    connected_ = false;
  }
  void OnTracingDisabled() override {
    svc_endpoint_->FreeBuffers();
    RotateFilenameAndEnable();
  }

  void RotateFilenameAndEnable();
  
  const FileWriterConsumerOptions options_;
  uint32_t file_num_ = 0;
  bool connected_ = false;
};

}
}

#endif