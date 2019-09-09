#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class BombChipAction : public ChipAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
public:
  BombChipAction(Character* owner, int damage);
  virtual ~BombChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};
