#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;

class AirHockeyCardAction : public CardAction {
private:
  sf::Sprite overlay;
  SpriteProxyNode* attachment{ nullptr };
  Animation attachmentAnim;
  int damage;
public:
  AirHockeyCardAction(std::shared_ptr<Character> actor, int damage);
  ~AirHockeyCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd();
  void OnActionEnd();
  void OnExecute(std::shared_ptr<Character> user);
};
