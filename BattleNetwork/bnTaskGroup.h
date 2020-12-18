#pragma once
#include <list>
#include "bnCallback.h"

class TaskGroup {
  std::list<std::pair<std::string, Callback<void()>>> tasks;
  unsigned currentTask{};
public:
  TaskGroup() = default;
  TaskGroup(TaskGroup&& other) noexcept;
  ~TaskGroup() = default;

  const bool HasMore() const;
  void DoNextTask();
  const std::string& GetTaskName() const;
  const unsigned GetTaskNumber() const;
  const unsigned GetTotalTasks() const;
  void AddTask(const std::string& name, Callback<void()>&& task);
};
