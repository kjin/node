#ifndef SRC_NODE_TRACE_WRITER_H_
#define SRC_NODE_TRACE_WRITER_H_

#include <sstream>
#include <queue>

#include "node_mutex.h"
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
  // Triggers callback to initiate writing the contents of stream_ to disk.
  uv_async_t flush_signal_;
  // Triggers callback to close async objects, ending the tracing thread.
  uv_async_t exit_signal_;
  // Prevents concurrent R/W on state related to serialized trace data
  // before it's written to disk, namely stream_ and total_traces_.
  Mutex stream_mutex_;
  // Prevents concurrent R/W on state related to write requests.
  Mutex request_mutex_;
  // Allows blocking calls to Flush() to wait on a condition for
  // trace events to be written to disk.
  ConditionVariable request_cond_;
  int fd_ = -1;
  std::queue<WriteRequest*> write_req_queue_;
  int num_write_requests_ = 0;
  int highest_request_id_completed_ = 0;
  int total_traces_ = 0;
  int file_num_ = 0;
  std::ostringstream stream_;
  uv_pipe_t trace_file_pipe_;
};

}  // namespace tracing
}  // namespace node

#endif  // SRC_NODE_TRACE_WRITER_H_
