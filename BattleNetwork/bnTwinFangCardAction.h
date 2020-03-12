#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class TwinFangCardAction : public CardAction {
private:
  sf::Sprite cannon;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  TwinFangCardAction(Character* owner, int damage);
  ~TwinFangCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
