#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class YoYoChipAction : public ChipAction {
private:
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  Entity* yoyo;
  int damage;
public:
  YoYoChipAction(Character* owner, int damage);
  ~YoYoChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
