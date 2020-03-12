#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class VulcanCardAction : public CardAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  VulcanCardAction(Character* owner, int damage);
  ~VulcanCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
