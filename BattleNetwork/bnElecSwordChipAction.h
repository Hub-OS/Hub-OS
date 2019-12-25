#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ElecSwordChipAction : public ChipAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  ElecSwordChipAction(Character* owner, int damage);
  ~ElecSwordChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};

