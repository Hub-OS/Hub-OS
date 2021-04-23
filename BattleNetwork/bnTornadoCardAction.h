#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class TornadoCardAction : public CardAction {
private:
  sf::Sprite fan;
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  bool armIsOut;
  int damage;
public:
  TornadoCardAction(Character& actor, int damage);
  ~TornadoCardAction();
  void Update(double _elapsed) override;
  void OnActionEnd() override;
  void OnExecute(Character*) override;
  void OnAnimationEnd() override;
}; 
