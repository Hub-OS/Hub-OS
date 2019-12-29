#pragma once
#include "bnSwordChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ElecSwordChipAction : public SwordChipAction {
public:
  ElecSwordChipAction(Character* owner, int damage);
  ~ElecSwordChipAction();
  void OnSpawnHitbox();
};

