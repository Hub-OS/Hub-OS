#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class BombChipAction : public ChipAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  BombChipAction(Character* owner, int damage);
  ~BombChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
