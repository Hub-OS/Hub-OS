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
  ThunderCardAction(Character& owner, int damage);
  ~ThunderCardAction();

  void OnAnimationEnd() override;
  void OnEndAction() override;
  void OnExecute() override;
};
