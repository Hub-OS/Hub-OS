#pragma once

class Entity;
class BattleScene;

/* 
Components need to be genric enough to be used by all in-battle entities
*/

class Component {
private: 
  Entity* owner;

public:
  Component(Entity* owner) { this->owner = owner; };
  ~Component() { ; }

  Component(Component&& rhs) = delete;
  Component(const Component& rhs) = delete;

  Entity* GetOwner() { return owner;  }
  virtual void Update(float _elapsed) = 0;
  virtual void Inject(BattleScene&) = 0;
};