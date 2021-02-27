#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class BusterCardAction : public CardAction {
private:
  SpriteProxyNode* buster{ nullptr }, * flare{ nullptr };
  Attachment* busterAttachment{ nullptr };
  Animation busterAnim, flareAnim;
  Entity::RemoveCallback* busterRemoved{ nullptr };
  bool charged{};
  int damage{};
  bool isBusterAlive{};
public:
  BusterCardAction(Character& user, bool charged, int damage);
  ~BusterCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd();
  void OnEndAction();
  void OnExecute();
};
