#pragma once

#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ThunderCardAction : public CardAction {
private:
  SpriteProxyNode *attachment;
  Animation attachmentAnim;
  int damage;
public:
  ThunderCardAction(Character* actor, int damage);
  ~ThunderCardAction();

  void OnAnimationEnd() override;
  void OnActionEnd() override;
  void OnExecute(Character* user) override;
};
