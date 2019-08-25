#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class AirShotChipAction : public ChipAction {
private:
  sf::Sprite airshot;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
public:
  AirShotChipAction(Character* owner, int damage);
  virtual ~AirShotChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};