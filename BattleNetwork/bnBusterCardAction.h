#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class BusterCardAction : public CardAction {
private:
  SpriteProxyNode *buster, *flare;
  Animation busterAnim, flareAnim;
  bool charged;
  int damage;
  bool isBusterAlive;
public:
  BusterCardAction(Character& user, bool charged, int damage);
  ~BusterCardAction();
  void OnEndAction() override;
  void OnExecute() override;
};
