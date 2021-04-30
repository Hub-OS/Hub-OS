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
  EntityRemoveCallback* busterRemoved{ nullptr };
  bool charged{};
  int damage{};
public:
  BusterCardAction(Character& user, bool charged, int damage);
  ~BusterCardAction();

  void Update(double _elapsed) override;
  void OnAnimationEnd();
  void OnActionEnd();
  void OnExecute(Character* user);
};
