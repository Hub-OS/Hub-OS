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
  int damage;
public:
  AirShotChipAction(Character* owner, int damage);
  ~AirShotChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};