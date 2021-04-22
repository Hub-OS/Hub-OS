#pragma once
#include "bnSwordCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class LongSwordCardAction : public SwordCardAction {
public:
  LongSwordCardAction(Character& actor, int damage);
  ~LongSwordCardAction();
  void OnSpawnHitbox(Entity::ID_t userId) override;
};

