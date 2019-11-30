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
public:
  ElecSwordChipAction(Character* owner, int damage);
  virtual ~ElecSwordChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};

