#pragma once

#include <list>
#include <string>
#include <thread>

#include "bnTaskGroup.h"
#include "bnScene.h"

/** @brief this scene represents a bunch of tasks performing in the background
*
* Loader scenes are scenes that might have "Now Loading..." text appear while 
* an animation plays with a % number of progress on the task. 
* These scenes require a TaskGroup input and will report to the scene
* what % is done along with the task name. LoaderScenes can choose display this data
* in any form that they'd like
*/

class TaskGroup;

class LoaderScene : public Scene {
private:
  TaskGroup tasks;
  std::thread taskThread;

  void ExecuteTasks();
public:
  LoaderScene(swoosh::ActivityController& controller, TaskGroup&& tasks);
  virtual ~LoaderScene();

  const bool IsComplete() const;
  void LaunchTasks();

  virtual void onTaskComplete(const std::string& taskName, float progress) = 0;
  virtual void onTaskBegin(const std::string& taskName, float progress) = 0;
};