#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class LightningCardAction : public CardAction {
private:
  sf::Sprite buster;
  SpriteProxyNode* attachment, * attack;
  Animation attachmentAnim, attackAnim;
  int damage;
  bool fired{ false };
  bool stun{ true };
public:
  LightningCardAction(Character* owner, int damage);
  ~LightningCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd();
  void EndAction();
  void Execute();
  void SetStun(bool stun);
};