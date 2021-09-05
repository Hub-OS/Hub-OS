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
  LightningCardAction(std::shared_ptr<Character> actor, int damage);
  ~LightningCardAction();
  void Update(double _elapsed) override;
  void OnAnimationEnd();
  void OnActionEnd();
  void OnExecute(std::shared_ptr<Character> user);
  void SetStun(bool stun);
};