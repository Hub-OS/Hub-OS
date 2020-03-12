#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ThunderCardAction : public CardAction {
private:
  SpriteSceneNode *attachment;
  Animation attachmentAnim;
  int damage;
public:
  ThunderCardAction(Character* owner, int damage);
  ~ThunderCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
