#include "tracing/node_trace_writer.h"

#include <string.h>

#include "trace_event_common.h"
#include "util.h"

namespace node {
namespace tracing {

NodeTraceWriter::NodeTraceWriter() {
  int err = uv_loop_init(&tracing_loop_);
  CHECK_EQ(err, 0);

  flush_signal_.data = this;
  err = uv_async_init(&tracing_loop_, &flush_signal_, FlushSignalCb);
  CHECK_EQ(err, 0);

  exit_signal_.data = this;
  err = uv_async_init(&tracing_loop_, &exit_signal_, ExitSignalCb);
  CHECK_EQ(err, 0);

  // synchronization variables setup
  err = uv_mutex_init(&stream_mutex_);
  CHECK_EQ(err, 0);
  err = uv_mutex_init(&request_mutex_);
  CHECK_EQ(err, 0);
  err = uv_cond_init(&writer_cond_);
  CHECK_EQ(err, 0);

  err = uv_thread_create(&thread_, ThreadCb, this);
  CHECK_EQ(err, 0);
}

void NodeTraceWriter::WriteSuffix() {
  // If our final log file has traces, then end the file appropriately.
  // This means that if no trace events are recorded, then no trace file is
  // produced.
  uv_mutex_lock(&stream_mutex_);
  if (total_traces_ > 0) {
    total_traces_ = 0; // so we don't write it again in FlushPrivate
    stream_ << "]}\n";
    uv_mutex_unlock(&stream_mutex_);
    Flush(true);
  } else {
    uv_mutex_unlock(&stream_mutex_);
  }
}

NodeTraceWriter::~NodeTraceWriter() {
  WriteSuffix();
  uv_fs_t req;
  int err;
  if (fd_ != -1) {
    err = uv_fs_close(&tracing_loop_, &req, fd_, nullptr);
    CHECK_EQ(err, 0);
    uv_fs_req_cleanup(&req);
    uv_close(reinterpret_cast<uv_handle_t*>(&trace_file_pipe_), nullptr);
  }
  // Thread should finish when the tracing loop is stopped.
  uv_async_send(&exit_signal_);
  uv_thread_join(&thread_);
}

void NodeTraceWriter::OpenNewFileForStreaming() {
  ++file_num_;
  uv_fs_t req;
  std::string log_file = "node_trace." + std::to_string(file_num_) + ".log";
  fd_ = uv_fs_open(&tracing_loop_, &req, log_file.c_str(),
      O_CREAT | O_WRONLY | O_TRUNC, 0644, NULL);
  CHECK_NE(fd_, -1);
  uv_fs_req_cleanup(&req);
  uv_pipe_init(&tracing_loop_, &trace_file_pipe_, 0);
  uv_pipe_open(&trace_file_pipe_, fd_);
}

void NodeTraceWriter::AppendTraceEvent(TraceObject* trace_event) {
  uv_mutex_lock(&stream_mutex_);
  // If this is the first trace event, open a new file for streaming.
  if (total_traces_ == 0) {
    OpenNewFileForStreaming();
    stream_ << "{\"traceEvents\":[";
  } else {
    stream_ << ",\n";
  }
  ++total_traces_;
  stream_ << "{\"pid\":" << trace_event->pid()
          << ",\"tid\":" << trace_event->tid()
          << ",\"ts\":" << trace_event->ts()
          << ",\"tts\":" << trace_event->tts() << ",\"ph\":\""
          << trace_event->phase() << "\",\"cat\":\""
          << TracingController::GetCategoryGroupName(
                 trace_event->category_enabled_flag())
          << "\",\"name\":\"" << trace_event->name()
          << "\",\"args\":{},\"dur\":" << trace_event->duration()
          << ",\"tdur\":" << trace_event->cpu_duration();
  if (trace_event->flags() & TRACE_EVENT_FLAG_HAS_ID) {
    if (trace_event->scope() != nullptr) {
      stream_ << ",\"scope\":\"" << trace_event->scope() << "\"";
    }
    stream_ << ",\"id\":" << trace_event->id();
  }
  stream_ << "}";
  uv_mutex_unlock(&stream_mutex_);
}

void NodeTraceWriter::FlushPrivate() {
  std::string str;
  int highest_request_id;
  uv_mutex_lock(&stream_mutex_);
  if (total_traces_ >= kTracesPerFile) {
    total_traces_ = 0;
    stream_ << "]}\n";
  }
  // str() makes a copy of the contents of the stream.
  str = stream_.str();
  stream_.str("");
  if (str.length() > 0 && fd_ != -1) {
    uv_mutex_lock(&request_mutex_);
    highest_request_id = num_write_requests_++;
    uv_mutex_unlock(&request_mutex_);
    uv_mutex_unlock(&stream_mutex_);
    WriteToFile(str, highest_request_id);
  } else {
    // Empty string or file not opened - so don't write.
    // (Both of these are true if no trace events were recorded.)
    uv_mutex_unlock(&stream_mutex_);
  }
}

void NodeTraceWriter::FlushSignalCb(uv_async_t* signal) {
  NodeTraceWriter* trace_writer = static_cast<NodeTraceWriter*>(signal->data);
  trace_writer->FlushPrivate();
}

// TODO: Remove (is it necessary to change the API? Since because of WriteSuffix
// it no longer matters whether it's true or false)
void NodeTraceWriter::Flush() {
  Flush(true);
}

void NodeTraceWriter::Flush(bool blocking) {
  uv_mutex_lock(&request_mutex_);
  int request_id = num_write_requests_++;
  uv_mutex_unlock(&request_mutex_);
  int err = uv_async_send(&flush_signal_);
  CHECK_EQ(err, 0);
  if (blocking) {
    uv_mutex_lock(&request_mutex_);
    // Wait until data associated with this request id has been written to disk.
    // This guarantees that data from all earlier requests have also been
    // written.
    while (request_id > highest_request_id_completed_) {
      uv_cond_wait(&writer_cond_, &request_mutex_);
    }
    uv_mutex_unlock(&request_mutex_);
  }
}

void NodeTraceWriter::WriteToFile(std::string str, int highest_request_id) {
  const char* c_str = str.c_str();
  uv_buf_t uv_buf = uv_buf_init(const_cast<char*>(c_str), strlen(c_str));
  WriteRequest write_req;
  write_req.writer = this;
  write_req.highest_request_id = highest_request_id;
  uv_mutex_lock(&request_mutex_);
  // Manage a queue of WriteRequest objects because the behavior of uv_write is
  // is undefined if the same WriteRequest object is used more than once
  // between WriteCb calls. In addition, this allows us to keep track of the id
  // of the latest write request that actually been completed.
  write_req_queue_.push(write_req);
  uv_mutex_unlock(&request_mutex_);
  // TODO: Is the return value of back() guaranteed to always have the
  // same address?
  uv_write(reinterpret_cast<uv_write_t*>(&write_req_queue_.back()),
           reinterpret_cast<uv_stream_t*>(&trace_file_pipe_),
           &uv_buf, 1, WriteCb);
}

void NodeTraceWriter::WriteCb(uv_write_t* req, int status) {
  WriteRequest* write_req = reinterpret_cast<WriteRequest*>(req);
  NodeTraceWriter* writer = write_req->writer;
  int highest_request_id = write_req->highest_request_id;
  uv_mutex_lock(&writer->request_mutex_);
  CHECK_EQ(write_req, &writer->write_req_queue_.front());
  writer->write_req_queue_.pop();
  writer->highest_request_id_completed_ = highest_request_id;
  uv_mutex_unlock(&writer->request_mutex_);
  uv_cond_broadcast(&writer->writer_cond_);
}

// static
void NodeTraceWriter::ExitSignalCb(uv_async_t* signal) {
  NodeTraceWriter* trace_writer = static_cast<NodeTraceWriter*>(signal->data);
  uv_close(reinterpret_cast<uv_handle_t*>(&trace_writer->flush_signal_), nullptr);
  uv_close(reinterpret_cast<uv_handle_t*>(&trace_writer->exit_signal_), nullptr);
}

// static
void NodeTraceWriter::ThreadCb(void* agent) {
  NodeTraceWriter* writer = static_cast<NodeTraceWriter*>(agent);
  uv_run(&writer->tracing_loop_, UV_RUN_DEFAULT);
}

}  // namespace tracing
}  // namespace node
