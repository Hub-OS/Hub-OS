#pragma once
#include <Swoosh/Activity.h>
#include "bnResourceHandle.h"

class Scene : public swoosh::Activity, public ResourceHandle {
private:
  
public:
  Scene(swoosh::ActivityController* controller) : swoosh::Activity(controller){}
  Scene(const Scene& rhs) = delete;
  virtual ~Scene() = default;
};