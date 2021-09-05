#pragma once
#include "bnCardAction.h"

class InvisCardAction : public CardAction {
public:
  InvisCardAction(std::shared_ptr<Character> actor);
  ~InvisCardAction();

  void OnExecute(std::shared_ptr<Character> user) override;
  void OnActionEnd() override;
  void OnAnimationEnd() override;
};