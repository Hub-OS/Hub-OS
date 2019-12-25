#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class VulcanChipAction : public ChipAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  VulcanChipAction(Character* owner, int damage);
  ~VulcanChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
