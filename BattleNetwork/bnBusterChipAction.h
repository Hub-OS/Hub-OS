#pragma once

#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class BusterChipAction : public ChipAction {
private:
  SpriteSceneNode *attachment, *attachment2;
  Animation attachmentAnim, attachmentAnim2;
  bool charged;
  int damage;
  bool isBusterAlive;
public:
  BusterChipAction(Character* owner, bool charged, int damage);
  ~BusterChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
