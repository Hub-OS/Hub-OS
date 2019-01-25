#pragma once

#include "bnEntity.h"

class BattleScene;

/* 
Components need to be genric enough to be used by all in-battle entities
*/

class Component {
private: 
  Entity* owner;

public:
  Component() = delete;
  Component(Entity* owner) { this->owner = owner; owner->RegisterComponent(this); };
  ~Component() { ; }

  Component(Component&& rhs) = delete;
  Component(const Component& rhs) = delete;

  Entity* GetOwner() { return owner;  }
  void FreeOwner() { owner = nullptr; }

  virtual void Update(float _elapsed) = 0;
  virtual void Inject(BattleScene&) = 0;
};