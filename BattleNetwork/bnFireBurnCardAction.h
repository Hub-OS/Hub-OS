#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include "bnFireBurn.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class FireBurnCardAction : public CardAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  FireBurn::Type type;
  int damage;
public:
  FireBurnCardAction(Character* owner, FireBurn::Type type, int damage);
  ~FireBurnCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
