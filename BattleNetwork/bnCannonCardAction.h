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

  CannonCardAction(Character* user, Type type, int damage);
  ~CannonCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd();
  void OnExecute(Character* user);
};