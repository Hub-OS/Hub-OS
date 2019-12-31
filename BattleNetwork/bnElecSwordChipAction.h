#pragma once
#include "bnLongSwordChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ElecSwordChipAction : public LongSwordChipAction {
public:
  ElecSwordChipAction(Character* owner, int damage);
  ~ElecSwordChipAction();
};

