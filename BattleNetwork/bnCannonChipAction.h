#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class CannonChipAction : public ChipAction {
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

  CannonChipAction(Character* owner, int damage, Type type = Type::green);
  ~CannonChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};