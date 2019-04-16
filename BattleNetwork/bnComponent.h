#pragma once

#include "bnEntity.h"

class BattleScene;

/* 
Components need to be genric enough to be used by all in-battle entities
*/

class Component {
private: 
  Entity* owner;
  static long numOfComponents;
  long ID;

public:
  Component() = delete;
  Component(Entity* owner) { this->owner = owner; ID = ++numOfComponents;  };
  virtual ~Component() { ; }

  Component(Component&& rhs) = delete;
  Component(const Component& rhs) = delete;

  Entity* GetOwner() { return owner;  }

  template<typename T>
  T* GetOwnerAs() { return dynamic_cast<T*>(owner); }

  void FreeOwner() { owner = nullptr; }

  const long GetID() const { return ID; }

  virtual void Update(float _elapsed) = 0;
  virtual void Inject(BattleScene&) = 0;
};
