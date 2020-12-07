#pragma once
#include "bnSwordCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class WideSwordCardAction : public SwordCardAction {
public:
  WideSwordCardAction(Character& owner, int damage);
  ~WideSwordCardAction();
  void OnSpawnHitbox() override;
};

