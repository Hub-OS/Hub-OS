#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class TwinFangChipAction : public ChipAction {
private:
  sf::Sprite cannon;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
public:
  TwinFangChipAction(Character* owner, int damage);
  virtual ~TwinFangChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
};
