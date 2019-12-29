#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class RecoverChipAction : public ChipAction {
  int heal;

public:
  RecoverChipAction(Character* owner, int heal);
  ~RecoverChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
