#pragma once
#include "bnCallback.h"
class Entity;
class EntityRemoveCallback : public Callback<void(Entity*)> {
private:
  Entity& owner;
public:
  using Callback<void(Entity*)>::operator();

  EntityRemoveCallback(Entity& owner);
  ~EntityRemoveCallback();
};