#include "bnLoaderScene.h"

void LoaderScene::ExecuteTasks()
{
  while (tasks.HasMore()) {
    const std::string taskname = tasks.GetTaskName();
    float progress = tasks.GetTaskNumber() / static_cast<float>(tasks.GetTotalTasks());
    this->onTaskBegin(taskname, progress);
    tasks.DoNextTask();
    
    progress = tasks.GetTaskNumber() / static_cast<float>(tasks.GetTotalTasks());
    this->onTaskComplete(taskname, progress);
  }
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
  return tasks.GetTotalTasks() == 0;
}

void LoaderScene::LaunchTasks()
{
  taskThread = std::thread(std::bind(&LoaderScene::ExecuteTasks, this));
  taskThread.detach();
}
