#include "tracing/legacy_agent.h"

#include <string>
#include "trace_event.h"
#include "tracing/node_trace_buffer.h"
#include "debug_utils.h"
#include "env-inl.h"

namespace node {
namespace tracing {

class LegacyAgent::ScopedSuspendTracing {
 public:
  ScopedSuspendTracing(TracingController* controller, LegacyAgent* agent,
                       bool do_suspend = true)
    : controller_(controller), agent_(do_suspend ? agent : nullptr) {
    if (do_suspend) {
      CHECK(agent_->started_);
      controller->StopTracing();
    }
  }

  ~ScopedSuspendTracing() {
    if (agent_ == nullptr) return;
    TraceConfig* config = agent_->CreateTraceConfig();
    if (config != nullptr) {
      controller_->StartTracing(config);
    }
  }

 private:
  TracingController* controller_;
  LegacyAgent* agent_;
};

namespace {

std::set<std::string> flatten(
    const std::unordered_map<int, std::multiset<std::string>>& map) {
  std::set<std::string> result;
  for (const auto& id_value : map)
    result.insert(id_value.second.begin(), id_value.second.end());
  return result;
}

}  // namespace

using v8::platform::tracing::TraceConfig;
using v8::platform::tracing::TraceWriter;
using std::string;

LegacyAgent::LegacyAgent() : tracing_controller_(new TracingController()) {
  tracing_controller_->Initialize(nullptr);

  CHECK_EQ(uv_loop_init(&tracing_loop_), 0);
  CHECK_EQ(uv_async_init(&tracing_loop_,
                         &initialize_writer_async_,
                         [](uv_async_t* async) {
    LegacyAgent* agent = ContainerOf(&LegacyAgent::initialize_writer_async_, async);
    agent->InitializeWritersOnThread();
  }), 0);
  uv_unref(reinterpret_cast<uv_handle_t*>(&initialize_writer_async_));
}

void LegacyAgent::InitializeWritersOnThread() {
  Mutex::ScopedLock lock(initialize_writer_mutex_);
  while (!to_be_initialized_.empty()) {
    AsyncTraceWriter* head = *to_be_initialized_.begin();
    head->InitializeOnThread(&tracing_loop_);
    to_be_initialized_.erase(head);
  }
  initialize_writer_condvar_.Broadcast(lock);
}

LegacyAgent::~LegacyAgent() {
  categories_.clear();
  writers_.clear();

  StopTracing();

  uv_close(reinterpret_cast<uv_handle_t*>(&initialize_writer_async_), nullptr);
  uv_run(&tracing_loop_, UV_RUN_ONCE);
  CheckedUvLoopClose(&tracing_loop_);
}

void LegacyAgent::Start() {
  if (started_)
    return;

  NodeTraceBuffer* trace_buffer_ = new NodeTraceBuffer(
      NodeTraceBuffer::kBufferChunks, this, &tracing_loop_);
  tracing_controller_->Initialize(trace_buffer_);

  // This thread should be created *after* async handles are created
  // (within NodeTraceWriter and NodeTraceBuffer constructors).
  // Otherwise the thread could shut down prematurely.
  CHECK_EQ(0, uv_thread_create(&thread_, [](void* arg) {
    LegacyAgent* agent = static_cast<LegacyAgent*>(arg);
    uv_run(&agent->tracing_loop_, UV_RUN_DEFAULT);
  }, this));
  started_ = true;
}

std::unique_ptr<AgentWriterHandle> LegacyAgent::AddClient(
    const std::set<std::string>& categories,
    std::unique_ptr<AsyncTraceWriter> writer,
    enum UseDefaultCategoryMode mode) {
  Start();

  const std::set<std::string>* use_categories = &categories;

  std::set<std::string> categories_with_default;
  if (mode == kUseDefaultCategories) {
    categories_with_default.insert(categories.begin(), categories.end());
    categories_with_default.insert(categories_[kDefaultHandleId].begin(),
                                   categories_[kDefaultHandleId].end());
    use_categories = &categories_with_default;
  }

  ScopedSuspendTracing suspend(tracing_controller_.get(), this);
  int id = next_writer_id_++;
  AsyncTraceWriter* raw = writer.get();
  writers_[id] = std::move(writer);
  categories_[id] = { use_categories->begin(), use_categories->end() };

  {
    Mutex::ScopedLock lock(initialize_writer_mutex_);
    to_be_initialized_.insert(raw);
    uv_async_send(&initialize_writer_async_);
    while (to_be_initialized_.count(raw) > 0)
      initialize_writer_condvar_.Wait(lock);
  }

  return std::unique_ptr<AgentWriterHandle>(new LegacyAgentWriterHandle(this, id));
}

std::unique_ptr<AgentWriterHandle> LegacyAgent::DefaultHandle() {
  return std::unique_ptr<AgentWriterHandle>(new LegacyAgentWriterHandle(this, kDefaultHandleId));
}

void LegacyAgent::StopTracing() {
  if (!started_)
    return;
  // Perform final Flush on TraceBuffer. We don't want the tracing controller
  // to flush the buffer again on destruction of the V8::Platform.
  tracing_controller_->StopTracing();
  tracing_controller_->Initialize(nullptr);
  started_ = false;

  // Thread should finish when the tracing loop is stopped.
  uv_thread_join(&thread_);
}

void LegacyAgent::Disconnect(int client) {
  if (client == kDefaultHandleId) return;
  {
    Mutex::ScopedLock lock(initialize_writer_mutex_);
    to_be_initialized_.erase(writers_[client].get());
  }
  ScopedSuspendTracing suspend(tracing_controller_.get(), this);
  writers_.erase(client);
  categories_.erase(client);
}

void LegacyAgent::Enable(int id, const std::set<std::string>& categories) {
  if (categories.empty())
    return;

  ScopedSuspendTracing suspend(tracing_controller_.get(), this,
                               id != kDefaultHandleId);
  categories_[id].insert(categories.begin(), categories.end());
}

void LegacyAgent::Disable(int id, const std::set<std::string>& categories) {
  ScopedSuspendTracing suspend(tracing_controller_.get(), this,
                               id != kDefaultHandleId);
  std::multiset<std::string>& writer_categories = categories_[id];
  for (const std::string& category : categories) {
    auto it = writer_categories.find(category);
    if (it != writer_categories.end())
      writer_categories.erase(it);
  }
}

TraceConfig* LegacyAgent::CreateTraceConfig() const {
  if (categories_.empty())
    return nullptr;
  TraceConfig* trace_config = new TraceConfig();
  for (const auto& category : flatten(categories_)) {
    trace_config->AddIncludedCategory(category.c_str());
  }
  return trace_config;
}

std::string LegacyAgent::GetEnabledCategories() const {
  std::string categories;
  for (const std::string& category : flatten(categories_)) {
    if (!categories.empty())
      categories += ',';
    categories += category;
  }
  return categories;
}

void LegacyAgent::AppendTraceEvent(TraceObject* trace_event) {
  for (const auto& id_writer : writers_)
    id_writer.second->AppendTraceEvent(trace_event);
}

void LegacyAgent::AddMetadataEvent(std::unique_ptr<TraceObject> event) {
  Mutex::ScopedLock lock(metadata_events_mutex_);
  metadata_events_.push_back(std::move(event));
}

void LegacyAgent::Flush(bool blocking) {
  {
    Mutex::ScopedLock lock(metadata_events_mutex_);
    for (const auto& event : metadata_events_)
      AppendTraceEvent(event.get());
  }

  for (const auto& id_writer : writers_)
    id_writer.second->Flush(blocking);
}

}  // namespace tracing
}  // namespace node
