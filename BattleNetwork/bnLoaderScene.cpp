#include "bnLoaderScene.h"

void LoaderScene::ExecuteTasks()
{
  while (tasks.HasMore()) {
    float progress = tasks.GetTaskNumber() / static_cast<float>(tasks.GetTotalTasks());
    this->onTaskBegin(tasks.GetTaskName(), progress);
    tasks.DoNextTask();
    
    progress = tasks.GetTaskNumber() / static_cast<float>(tasks.GetTotalTasks());
    this->onTaskComplete(tasks.GetTaskName(), progress);
  }
}

LoaderScene::LoaderScene(swoosh::ActivityController * controller, TaskGroup && tasks)
: tasks(std::move(tasks)),
  Scene(controller) {
  completion = 0;
}

LoaderScene::~LoaderScene()
{
  taskThread.join();
}

const bool LoaderScene::IsComplete() const
{
  return static_cast<int>(completion) == 1;
}

void LoaderScene::LaunchTasks()
{
  taskThread = std::thread(std::bind(&LoaderScene::ExecuteTasks, this));
  taskThread.detach();
}
