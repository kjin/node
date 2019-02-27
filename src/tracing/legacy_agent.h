#ifndef SRC_TRACING_LEGACY_AGENT_H_
#define SRC_TRACING_LEGACY_AGENT_H_

#include "agent.h"

#include <list>
#include <unordered_map>

namespace node {
namespace tracing {

class LegacyAgent;

class LegacyAgentWriterHandle : public AgentWriterHandle {
 public:
  inline LegacyAgentWriterHandle() {}
  inline ~LegacyAgentWriterHandle() { reset(); }

  inline LegacyAgentWriterHandle(LegacyAgentWriterHandle&& other);
  inline LegacyAgentWriterHandle& operator=(LegacyAgentWriterHandle&& other);
  inline bool empty() const override { return agent_ == nullptr; }
  inline void reset() override;

  inline void Enable(const std::set<std::string>& categories) override;
  inline void Disable(const std::set<std::string>& categories) override;

  inline bool IsDefaultHandle() override;

  inline Agent* agent() override;

  inline v8::TracingController* GetTracingController() override;

  LegacyAgentWriterHandle(const LegacyAgentWriterHandle& other) = delete;
  LegacyAgentWriterHandle& operator=(const LegacyAgentWriterHandle& other) = delete;

 private:
  inline LegacyAgentWriterHandle(LegacyAgent* agent, int id) : agent_(agent), id_(id) {}

  LegacyAgent* agent_ = nullptr;
  int id_;

  friend class LegacyAgent;
};

class LegacyAgent : public Agent {
 public:
  LegacyAgent();
  ~LegacyAgent();

  TracingController* GetTracingController() override {
    TracingController* controller = tracing_controller_.get();
    CHECK_NOT_NULL(controller);
    return controller;
  }

  // Destroying the handle disconnects the client
  std::unique_ptr<AgentWriterHandle> AddClient(const std::set<std::string>& categories,
                              std::unique_ptr<AsyncTraceWriter> writer,
                              enum UseDefaultCategoryMode mode) override;
  // A handle that is only used for managing the default categories
  // (which can then implicitly be used through using `USE_DEFAULT_CATEGORIES`
  // when adding a client later).
  std::unique_ptr<AgentWriterHandle> DefaultHandle() override;

  // Returns a comma-separated list of enabled categories.
  std::string GetEnabledCategories() const override;

  // Writes to all writers registered through AddClient().
  void AppendTraceEvent(TraceObject* trace_event) override;

  void AddMetadataEvent(std::unique_ptr<TraceObject> event) override;
  // Flushes all writers registered through AddClient().
  void Flush(bool blocking) override;

  TraceConfig* CreateTraceConfig() const override;

 private:
  friend class LegacyAgentWriterHandle;

  void InitializeWritersOnThread();

  void Start();
  void StopTracing();
  void Disconnect(int client);

  void Enable(int id, const std::set<std::string>& categories);
  void Disable(int id, const std::set<std::string>& categories);

  uv_thread_t thread_;
  uv_loop_t tracing_loop_;

  bool started_ = false;
  class ScopedSuspendTracing;

  // Each individual Writer has one id.
  int next_writer_id_ = 1;
  enum { kDefaultHandleId = -1 };
  // These maps store the original arguments to AddClient(), by id.
  std::unordered_map<int, std::multiset<std::string>> categories_;
  std::unordered_map<int, std::unique_ptr<AsyncTraceWriter>> writers_;
  std::unique_ptr<TracingController> tracing_controller_;

  // Variables related to initializing per-event-loop properties of individual
  // writers, such as libuv handles.
  Mutex initialize_writer_mutex_;
  ConditionVariable initialize_writer_condvar_;
  uv_async_t initialize_writer_async_;
  std::set<AsyncTraceWriter*> to_be_initialized_;

  Mutex metadata_events_mutex_;
  std::list<std::unique_ptr<TraceObject>> metadata_events_;
};

void LegacyAgentWriterHandle::reset() {
  if (agent_ != nullptr)
    agent_->Disconnect(id_);
  agent_ = nullptr;
}

LegacyAgentWriterHandle& LegacyAgentWriterHandle::operator=(LegacyAgentWriterHandle&& other) {
  reset();
  agent_ = other.agent_;
  id_ = other.id_;
  other.agent_ = nullptr;
  return *this;
}

LegacyAgentWriterHandle::LegacyAgentWriterHandle(LegacyAgentWriterHandle&& other) {
  *this = std::move(other);
}

void LegacyAgentWriterHandle::Enable(const std::set<std::string>& categories) {
  if (agent_ != nullptr) agent_->Enable(id_, categories);
}

void LegacyAgentWriterHandle::Disable(const std::set<std::string>& categories) {
  if (agent_ != nullptr) agent_->Disable(id_, categories);
}

bool LegacyAgentWriterHandle::IsDefaultHandle() {
  return agent_ != nullptr && id_ == LegacyAgent::kDefaultHandleId;
}

Agent* LegacyAgentWriterHandle::agent() { return agent_; }

inline v8::TracingController* LegacyAgentWriterHandle::GetTracingController() {
  return agent_ != nullptr ? agent_->GetTracingController() : nullptr;
}

}  // namespace tracing
}  // namespace node

#endif  // SRC_TRACING_LEGACY_AGENT_H_
