#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class AirShotCardAction : public CardAction {
private:
  sf::Sprite airshot;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  AirShotCardAction(Character* owner, int damage);
  ~AirShotCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};