#pragma once
#include "bnCardAction.h"

class InvalidCardAction : public CardAction {
public:
  InvalidCardAction(std::shared_ptr<Character> actor);
  ~InvalidCardAction();

  void OnExecute(std::shared_ptr<Character> user) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
};