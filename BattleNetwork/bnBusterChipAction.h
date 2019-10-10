#pragma once

#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class BusterChipAction : public ChipAction {
private:
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
public:
  BusterChipAction(Character* owner, bool charged, int damage);
  ~BusterChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
};
