#ifndef SRC_TRACING_FILE_WRITER_CONSUMER_H_
#define SRC_TRACING_FILE_WRITER_CONSUMER_H_

#include "tracing/base/node_tracing.h"
#include <set>

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
class FileWriterConsumer : public base::NodeConsumer {
 public:
  FileWriterConsumer(FileWriterConsumerOptions& options): options_(options) {}
  void Enable(const std::set<std::string>& categories);
  void Disable(const std::set<std::string>& categories);
 private:
  void OnTracingDisabled() override {
    GetServiceEndpoint()->FreeBuffers();
    RotateFilenameAndEnable();
  }

  void RotateFilenameAndEnable();
  
  const FileWriterConsumerOptions options_;
  uint32_t file_num_ = 0;
};

}
}

#endif