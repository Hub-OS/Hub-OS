#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class BombCardAction : public CardAction {
private:
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  BombCardAction(Character& actor, int damage);
  ~BombCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(Character* user) override;
};
