#include "bnLoaderScene.h"

void LoaderScene::ExecuteTasks()
{
  while (tasks.HasMore()) {
    const std::string taskname = tasks.GetTaskName();
    const unsigned int tasknumber = tasks.GetTaskNumber();
    float progress = tasknumber / static_cast<float>(tasks.GetTotalTasks());

    mutex.lock();
    events.push([=] { this->onTaskBegin(taskname, progress); });
    mutex.unlock();

    tasks.DoNextTask();
    progress = tasknumber / static_cast<float>(tasks.GetTotalTasks());

    mutex.lock();
    events.push([=] { this->onTaskComplete(taskname, progress); });

    mutex.unlock();
  }
  isComplete = true;
}

LoaderScene::LoaderScene(swoosh::ActivityController& controller, TaskGroup && tasks) : 
  tasks(std::move(tasks)),
  Scene(controller) {
}

LoaderScene::~LoaderScene()
{
  if (taskThread.joinable()) {
    taskThread.join();
  }
}

const bool LoaderScene::IsComplete() const
{
  return isComplete;
}

void LoaderScene::LaunchTasks()
{
  taskThread = std::thread(std::bind(&LoaderScene::ExecuteTasks, this));
  taskThread.detach();
}

void LoaderScene::Poll()
{
  std::lock_guard lock(mutex);

  if (events.size()) {
    events.front()();
    events.pop();
  }
}
