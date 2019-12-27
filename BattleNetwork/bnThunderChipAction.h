#pragma once

#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ThunderChipAction : public ChipAction {
private:
  SpriteSceneNode *attachment;
  Animation attachmentAnim;
  int damage;
public:
  ThunderChipAction(Character* owner, int damage);
  ~ThunderChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
