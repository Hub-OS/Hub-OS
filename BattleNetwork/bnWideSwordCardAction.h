#pragma once
#include "bnSwordCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class WideSwordCardAction : public SwordCardAction {
public:
  WideSwordCardAction(Character* actor, int damage);
  ~WideSwordCardAction();
  void OnSpawnHitbox(Entity::ID_t userId) override;
};

