#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class YoYoCardAction : public CardAction {
private:
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  Entity* yoyo;
  int damage;
public:
  YoYoCardAction(Character* owner, int damage);
  ~YoYoCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
