#pragma once
#include "bnSwordChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class WideSwordChipAction : public SwordChipAction {
public:
  WideSwordChipAction(Character* owner, int damage);
  ~WideSwordChipAction();
  void OnSpawnHitbox();
};

