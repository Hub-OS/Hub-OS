#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class CannonChipAction : public ChipAction {
private:
  sf::Sprite cannon;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  CannonChipAction(Character* owner, int damage);
  ~CannonChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};