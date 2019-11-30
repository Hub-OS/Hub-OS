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
public:
  YoYoChipAction(Character* owner, int damage);
  virtual ~YoYoChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};
