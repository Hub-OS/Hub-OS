#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class CannonCardAction : public CardAction {
public:
  enum class Type : int {
    green,
    blue,
    red
  };

private:
  sf::Sprite cannon;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
  Type type;
public:

  CannonCardAction(Character* owner, int damage, Type type = Type::green);
  ~CannonCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};