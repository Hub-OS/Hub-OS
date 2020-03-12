#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class BombCardAction : public CardAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  BombCardAction(Character* owner, int damage);
  ~BombCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
