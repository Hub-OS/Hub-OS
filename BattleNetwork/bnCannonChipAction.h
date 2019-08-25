#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class CannonChipAction : public ChipAction {
private:
  sf::Sprite cannon;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
public:
  CannonChipAction(Character* owner, int damage);
  virtual ~CannonChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};