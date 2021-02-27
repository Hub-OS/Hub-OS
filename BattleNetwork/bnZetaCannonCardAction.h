#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnUIComponent.h"
#include "bnInputHandle.h"
#include "bnFont.h"
#include "bnText.h"

class SpriteProxyNode;
class Character;
class ZetaCannonCardAction : public CardAction, public InputHandle {
public:
  int damage{ 0 };
  double timer{ 5.0 }; // 5 seconds
  Font font;
  mutable Text timerLabel; // on the screen somewhere
  bool firstTime{ true };
  DefenseRule* defense{ nullptr };
public:

  ZetaCannonCardAction(Character& owner, int damage);
  ~ZetaCannonCardAction();
  void Update(double _elapsed) override final;
  void OnAnimationEnd() override final;
  void OnEndAction() override final;
  void OnExecute() override final;
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override final;
};