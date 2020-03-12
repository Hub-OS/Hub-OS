#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ReflectCardAction : public CardAction {
  int damage;

public:
  ReflectCardAction(Character* owner, int damage);
  ~ReflectCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
