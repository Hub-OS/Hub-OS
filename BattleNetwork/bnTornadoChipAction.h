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
  int damage;
public:
  TornadoChipAction(Character* owner, int damage);
  ~TornadoChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
}; 
