#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class FireBurnChipAction : public ChipAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
public:
  FireBurnChipAction(Character* owner, int damage);
  virtual ~FireBurnChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};
