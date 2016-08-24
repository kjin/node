#ifndef SRC_NODE_TRACE_WRITER_H_
#define SRC_NODE_TRACE_WRITER_H_

#include <sstream>
#include <queue>

#include "libplatform/v8-tracing.h"
#include "uv.h"

namespace node {
namespace tracing {

using v8::platform::tracing::TraceObject;
using v8::platform::tracing::TraceWriter;
using v8::platform::tracing::TracingController;

class NodeTraceWriter : public TraceWriter {
 public:
  NodeTraceWriter();
  ~NodeTraceWriter();

  void AppendTraceEvent(TraceObject* trace_event) override;
  void Flush() override;
  void Flush(bool blocking);

  static const int kTracesPerFile = 1 << 20;

 private:
  struct WriteRequest {
    uv_write_t req;
    NodeTraceWriter* writer;
    int highest_request_id;
  };

  static void WriteCb(uv_write_t* req, int status);
  void OpenNewFileForStreaming();
  void WriteToFile(std::string str, int highest_request_id);
  void WriteSuffix();
  static void ThreadCb(void* agent);
  static void FlushSignalCb(uv_async_t* signal);
  void FlushPrivate();
  static void ExitSignalCb(uv_async_t* signal);

  uv_thread_t thread_;
  uv_loop_t tracing_loop_;
  uv_async_t flush_signal_;
  uv_async_t exit_signal_;
  std::queue<WriteRequest> write_req_queue_;
  int num_write_requests_ = 0;
  int highest_request_id_completed_ = 0;
  uv_cond_t writer_cond_;
  uv_mutex_t stream_mutex_;
  uv_mutex_t request_mutex_;
  int fd_ = -1;
  int total_traces_ = 0;
  int file_num_ = 0;
  uv_pipe_t trace_file_pipe_;
  std::ostringstream stream_;
};

}  // namespace tracing
}  // namespace node

#endif  // SRC_NODE_TRACE_WRITER_H_
