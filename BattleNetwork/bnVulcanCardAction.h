#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class VulcanCardAction : public CardAction {
private:
  sf::Sprite overlay;
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  VulcanCardAction(Character* owner, int damage);
  ~VulcanCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
