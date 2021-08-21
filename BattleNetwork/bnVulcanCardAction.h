#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class VulcanCardAction : public CardAction {
private:
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  int damage;
public:
  VulcanCardAction(Character* actor, int damage);
  ~VulcanCardAction();
  void Update(double _elapsed) override final;
  void OnAnimationEnd() override final;
  void OnActionEnd() override;
  void OnExecute(Character* user) override;
};
