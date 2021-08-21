#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnUIComponent.h"
#include "bnInputHandle.h"
#include "bnFont.h"
#include "bnText.h"

class SpriteProxyNode;
class Character;
class DefenseRule;

class ZetaCannonCardAction : public CardAction, public InputHandle {
public:
  int damage{ 0 };
  double timer{ 5.0 }; // 5 seconds
  Font font;
  mutable Text timerLabel; // on the screen somewhere
  bool hide{};
  bool showTimerText{ false };
  DefenseRule* defense{ nullptr };
public:

  ZetaCannonCardAction(Character* actor, int damage);
  ~ZetaCannonCardAction();
  void Update(double _elapsed) override final;
  void OnAnimationEnd() override final;
  void OnActionEnd() override final;
  void OnExecute(Character*) override final;
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override final;
};