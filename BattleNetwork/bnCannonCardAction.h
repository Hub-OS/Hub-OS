#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class CannonCardAction : public CardAction {
public:
  enum class Type : int {
    green,
    blue,
    red
  };

private:
  sf::Sprite cannon;
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
  Type type;
public:

  CannonCardAction(Character* owner, int damage, Type type = Type::green);
  ~CannonCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};