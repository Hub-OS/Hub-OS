#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ReflectChipAction : public ChipAction {
  int damage;

public:
  ReflectChipAction(Character* owner, int damage);
  ~ReflectChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
