#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class BusterCardAction : public CardAction {
private:
  SpriteProxyNode *buster, *flare;
<<<<<<< HEAD
  Attachment* busterAttachment{ nullptr };
=======
>>>>>>> menu-refactor
  Animation busterAnim, flareAnim;
  bool charged;
  int damage;
  bool isBusterAlive;
public:
  BusterCardAction(Character& user, bool charged, int damage);
  ~BusterCardAction();
<<<<<<< HEAD
  void OnUpdate(float _elapsed);
  void OnAnimationEnd();
  void EndAction();
  void Execute();
=======
  void OnEndAction() override;
  void OnExecute() override;
>>>>>>> menu-refactor
};
