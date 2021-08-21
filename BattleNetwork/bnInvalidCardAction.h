#pragma once
#include "bnCardAction.h"

class InvalidCardAction : public CardAction {
public:
  InvalidCardAction(Character* actor);
  ~InvalidCardAction();

  void OnExecute(Character* user) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
};