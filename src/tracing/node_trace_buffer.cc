#include "tracing/node_trace_buffer.h"

namespace node {
namespace tracing {

InternalTraceBuffer::InternalTraceBuffer(size_t max_chunks,
    NodeTraceWriter* trace_writer, NodeTraceBuffer* external_buffer)
    : flushing_(false), max_chunks_(max_chunks), trace_writer_(trace_writer), external_buffer_(external_buffer) {
  chunks_.resize(max_chunks);
}

TraceObject* InternalTraceBuffer::AddTraceEvent(uint64_t* handle) {
  Mutex::ScopedLock scoped_lock(mutex_);
  // Create new chunk if last chunk is full or there is no chunk.
  if (total_chunks_ == 0 || chunks_[total_chunks_ - 1]->IsFull()) {
    auto& chunk = chunks_[total_chunks_++];
    if (chunk) {
      chunk->Reset(external_buffer_->current_chunk_seq_++);
    } else {
      chunk.reset(new TraceBufferChunk(external_buffer_->current_chunk_seq_++));
    }
  }
  auto& chunk = chunks_[total_chunks_ - 1];
  size_t event_index;
  TraceObject* trace_object = chunk->AddTraceEvent(&event_index);
  *handle = MakeHandle(total_chunks_ - 1, chunk->seq(), event_index);
  return trace_object;
}

TraceObject* InternalTraceBuffer::GetEventByHandle(uint64_t handle) {
  Mutex::ScopedLock scoped_lock(mutex_);
  size_t chunk_index, event_index;
  uint32_t chunk_seq;
  ExtractHandle(handle, &chunk_index, &chunk_seq, &event_index);
  if (chunk_index >= total_chunks_) {
    return NULL;
  }
  auto& chunk = chunks_[chunk_index];
  if (chunk->seq() != chunk_seq) {
    return NULL;
  }
  return chunk->GetEventAt(event_index);
}

void InternalTraceBuffer::Flush(bool blocking) {
  {
    Mutex::ScopedLock scoped_lock(mutex_);
    if (total_chunks_ > 0) {
      flushing_ = true;
      for (size_t i = 0; i < total_chunks_; ++i) {
        auto& chunk = chunks_[i];
        for (size_t j = 0; j < chunk->size(); ++j) {
          trace_writer_->AppendTraceEvent(chunk->GetEventAt(j));
        }
      }
      total_chunks_ = 0;
      flushing_ = false;
    }
  }
  trace_writer_->Flush(blocking);
}

uint64_t InternalTraceBuffer::MakeHandle(
    size_t chunk_index, uint32_t chunk_seq, size_t event_index) const {
  return static_cast<uint64_t>(chunk_seq) * Capacity() +
         chunk_index * TraceBufferChunk::kChunkSize + event_index;
}

void InternalTraceBuffer::ExtractHandle(
    uint64_t handle, size_t* chunk_index,
    uint32_t* chunk_seq, size_t* event_index) const {
  *chunk_seq = static_cast<uint32_t>(handle / Capacity());
  size_t indices = handle % Capacity();
  *chunk_index = indices / TraceBufferChunk::kChunkSize;
  *event_index = indices % TraceBufferChunk::kChunkSize;
}

NodeTraceBuffer::NodeTraceBuffer(size_t max_chunks,
    NodeTraceWriter* trace_writer, uv_loop_t* tracing_loop)
    : tracing_loop_(tracing_loop), trace_writer_(trace_writer),
      buffer1_(max_chunks, trace_writer, this),
      buffer2_(max_chunks, trace_writer, this) {
  current_buf_.store(&buffer1_);

  flush_signal_.data = this;
  int err = uv_async_init(tracing_loop_, &flush_signal_, NonBlockingFlushSignalCb);
  CHECK_EQ(err, 0);

  exit_signal_.data = this;
  err = uv_async_init(tracing_loop_, &exit_signal_, ExitSignalCb);
  CHECK_EQ(err, 0);
}

NodeTraceBuffer::~NodeTraceBuffer() {
  uv_async_send(&exit_signal_);
}

TraceObject* NodeTraceBuffer::AddTraceEvent(uint64_t* handle) {
  // If the buffer is full, attempt to perform a flush.
  if (!TryLoadAvailableBuffer()) {
    *handle = 0;
    return nullptr;
  }
  return current_buf_.load()->AddTraceEvent(handle);
}

TraceObject* NodeTraceBuffer::GetEventByHandle(uint64_t handle) {
  return current_buf_.load()->GetEventByHandle(handle);
}

bool NodeTraceBuffer::Flush() {
  buffer1_.Flush(true);
  buffer2_.Flush(true);
}

// Attempts to set current_buf_ such that it references a buffer that can
// can write at least one trace event. If both buffers are unavailable this
// method returns false; otherwise it returns true.
bool NodeTraceBuffer::TryLoadAvailableBuffer() {
  InternalTraceBuffer* prev_buf = current_buf_.load();
  if (prev_buf->IsFull()) {
    uv_async_send(&flush_signal_); // trigger flush on a separate thread
    InternalTraceBuffer* other_buf = prev_buf == &buffer1_ ?
      &buffer2_ : &buffer1_;
    if (!other_buf->IsFull()) {
      current_buf_.store(other_buf);
    } else {
      return false;
    }
  }
  return true;
}

// static
void NodeTraceBuffer::NonBlockingFlushSignalCb(uv_async_t* signal) {
  NodeTraceBuffer* buffer = reinterpret_cast<NodeTraceBuffer*>(signal->data);
  if (buffer->buffer1_.IsFull() && !buffer->buffer1_.IsFlushing()) {
    buffer->buffer1_.Flush(false);
  }
  if (buffer->buffer2_.IsFull() && !buffer->buffer2_.IsFlushing()) {
    buffer->buffer2_.Flush(false);
  }
}

// static
void NodeTraceBuffer::ExitSignalCb(uv_async_t* signal) {
  NodeTraceBuffer* buffer = reinterpret_cast<NodeTraceBuffer*>(signal->data);
  uv_close(reinterpret_cast<uv_handle_t*>(&buffer->flush_signal_), nullptr);
  uv_close(reinterpret_cast<uv_handle_t*>(&buffer->exit_signal_), nullptr);
}

}  // namespace tracing
}  // namespace node
