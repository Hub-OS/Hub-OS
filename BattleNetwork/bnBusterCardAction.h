#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class BusterCardAction : public CardAction {
private:
  SpriteSceneNode *attachment, *attachment2;
  Animation attachmentAnim, attachmentAnim2;
  bool charged;
  int damage;
  bool isBusterAlive;
public:
  BusterCardAction(Character* owner, bool charged, int damage);
  ~BusterCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
