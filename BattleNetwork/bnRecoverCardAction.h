#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class RecoverCardAction : public CardAction {
  int heal;
public:
  RecoverCardAction(std::shared_ptr<Character> actor, int heal);
  ~RecoverCardAction();

  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(std::shared_ptr<Character> user) override;
};
