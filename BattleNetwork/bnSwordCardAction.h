#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;

class SwordCardAction : public CardAction {
protected:
  int damage{};
  Element element{ Element::none };
  SpriteProxyNode* blade{ nullptr };
  SpriteProxyNode* hilt{ nullptr };
  Animation bladeAnim, hiltAnim;
public:
  SwordCardAction(Character* actor, int damage);
  virtual ~SwordCardAction();

  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(Character* user) override;
  virtual void OnSpawnHitbox(Entity::ID_t);
  void SetElement(Element elem);
  const Element GetElement() const;
};

