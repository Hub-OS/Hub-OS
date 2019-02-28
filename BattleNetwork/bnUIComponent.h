#pragma once

#include "bnComponent.h"
#include "bnSceneNode.h"

class BattleScene;

/*
components used for UI lookup
*/

class UIComponent : public Component, public SceneNode {
private:

public:
  UIComponent() = delete;
  UIComponent(Entity* owner) : Component(owner) { ; }
  ~UIComponent() { ; }

  UIComponent(UIComponent&& rhs) = delete;
  UIComponent(const UIComponent& rhs) = delete;

  virtual void draw(sf::RenderTarget & target, sf::RenderStates states) const {
    SceneNode::draw(target, states);
  };

  virtual void Update(float _elapsed) = 0;
  virtual void Inject(BattleScene&) = 0;
}; 
