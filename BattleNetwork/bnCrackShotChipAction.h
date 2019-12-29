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
  int damage;
public:
  CrackShotChipAction(Character* owner, int damage);
  ~CrackShotChipAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
