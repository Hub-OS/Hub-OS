#include "bnTaskGroup.h"

TaskGroup::TaskGroup(TaskGroup && other)
{
  tasks = std::move(other.tasks);
  currentTask = std::move(other.currentTask);
  other.tasks.clear();
  other.currentTask = 0;
}

const bool TaskGroup::HasMore() const
{
  return tasks.size();
}

void TaskGroup::DoNextTask()
{
  if (currentTask > 0) {
    tasks.erase(tasks.begin());
  }

  currentTask++;

  tasks.begin()->second();
}

const std::string & TaskGroup::GetTaskName() const
{
  return tasks.begin()->first;
}

const unsigned TaskGroup::GetTaskNumber() const
{
  return currentTask;
}

const unsigned TaskGroup::GetTotalTasks() const
{
  return static_cast<unsigned>(tasks.size());
}

void TaskGroup::AddTask(const std::string & name, const Callback<void()>& task)
{
  tasks.insert(tasks.begin(), std::make_pair(name, task));
}