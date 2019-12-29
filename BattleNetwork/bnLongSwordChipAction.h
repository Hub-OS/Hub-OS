#pragma once
#include "bnSwordChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class LongSwordChipAction : public SwordChipAction {
public:
  LongSwordChipAction(Character* owner, int damage);
  ~LongSwordChipAction();
  void OnSpawnHitbox();
};

