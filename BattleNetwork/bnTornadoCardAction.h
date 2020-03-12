#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class TornadoCardAction : public CardAction {
private:
  sf::Sprite fan;
  SpriteSceneNode* attachment;
  Animation attachmentAnim;
  bool armIsOut;
  int damage;
public:
  TornadoCardAction(Character* owner, int damage);
  ~TornadoCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
}; 
