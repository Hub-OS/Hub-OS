#pragma once
#include "bnCardAction.h"

class InvisCardAction : public CardAction {
public:
  InvisCardAction(Character* actor);
  ~InvisCardAction();

  void OnExecute(Character* user) override;
  void OnActionEnd() override;
  void OnAnimationEnd() override;
};