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
  LightningCardAction(Character& owner, int damage);
  ~LightningCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd();
  void OnEndAction();
  void OnExecute();
  void SetStun(bool stun);
};