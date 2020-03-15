#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class TwinFangCardAction : public CardAction {
private:
  sf::Sprite cannon;
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  TwinFangCardAction(Character* owner, int damage);
  ~TwinFangCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
