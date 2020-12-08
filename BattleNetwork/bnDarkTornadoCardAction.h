#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class DarkTornadoCardAction : public CardAction {
private:
  sf::Sprite fan;
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  bool armIsOut;
  int damage;
public:
  DarkTornadoCardAction(Character& owner, int damage);
  ~DarkTornadoCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
}; 
