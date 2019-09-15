#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ReflectChipAction : public ChipAction {
public:
  ReflectChipAction(Character* owner, int damage);
  virtual ~ReflectChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};
