#pragma once
#include <Swoosh/Activity.h>
#include "bnResourceHandle.h"
#include "bnInputHandle.h"
#include "bnNetHandle.h"
#include "bnGame.h"

class Scene : 
  public swoosh::Activity,
  public ResourceHandle, 
  public InputHandle, 
  public NetHandle {
public:
  Scene(swoosh::ActivityController& controller) : swoosh::Activity(&controller){}
  Scene(const Scene& rhs) = delete;
  virtual ~Scene() = default;
  Game& getController() { return static_cast<Game&>(swoosh::Activity::getController()); }
};