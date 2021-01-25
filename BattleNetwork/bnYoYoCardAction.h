#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class YoYoCardAction : public CardAction {
private:
  SpriteProxyNode* attachment;
  Animation attachmentAnim;
  Entity* yoyo;
  int damage;
public:
  YoYoCardAction(Character& owner, int damage);
  ~YoYoCardAction();
  void OnAnimationEnd() override;
  void OnUpdate(double _elapsed) override;
  void OnEndAction() override;
  void OnExecute() override;
};
