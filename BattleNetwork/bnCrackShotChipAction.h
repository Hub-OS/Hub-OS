#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;

class CrackShotChipAction : public ChipAction {
private:
  sf::Sprite overlay;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
public:
  CrackShotChipAction(Character* owner, int damage);
  virtual ~CrackShotChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};
