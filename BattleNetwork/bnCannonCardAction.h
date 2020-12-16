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
  SpriteProxyNode* attachment{ nullptr };
  Animation attachmentAnim;
  int damage;
  Type type;
public:

  CannonCardAction(Character& user, int damage, Type type = Type::green);
  ~CannonCardAction();

  void OnUpdate(double _elapsed);
  void OnAnimationEnd() override;
  void OnEndAction();
  void OnExecute();
};