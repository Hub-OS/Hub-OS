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
public:
  VulcanChipAction(Character* owner, int damage);
  virtual ~VulcanChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};
