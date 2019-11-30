#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class TornadoChipAction : public ChipAction {
private:
  sf::Sprite fan;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  bool armIsOut;
public:
  TornadoChipAction(Character* owner, int damage);
  virtual ~TornadoChipAction();
  virtual void OnUpdate(float _elapsed);
  virtual void EndAction();
}; 
