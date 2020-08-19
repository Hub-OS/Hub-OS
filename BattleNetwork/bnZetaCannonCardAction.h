#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnUIComponent.h"

#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ZetaCannonCardAction : public CardAction {
public:
  int damage{ 0 };
  double timer{ 5.0 }; // 5 seconds
  mutable sf::Text timerLabel; // on the screen somewhere
  std::shared_ptr<sf::Font> font;
  bool firstTime{ true };
  DefenseRule* defense{ nullptr };
public:

  ZetaCannonCardAction(Character* owner, int damage);
  ~ZetaCannonCardAction();
  void OnUpdate(float _elapsed) override final;
  void OnAnimationEnd() override final;
  void EndAction() override final;
  void Execute() override final;
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override final;
};