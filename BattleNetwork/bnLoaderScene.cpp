#include "bnLoaderScene.h"

void LoaderScene::ExecuteTasks()
{
  while (tasks.HasMore()) {
    const std::string taskname = tasks.GetTaskName();
    const unsigned int tasknumber = tasks.GetTaskNumber();
    float progress = tasknumber / static_cast<float>(tasks.GetTotalTasks());
    this->onTaskBegin(taskname, progress);
    tasks.DoNextTask();
    
    progress = tasknumber / static_cast<float>(tasks.GetTotalTasks());
    this->onTaskComplete(taskname, progress);
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
