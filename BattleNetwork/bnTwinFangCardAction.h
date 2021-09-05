#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class TwinFangCardAction : public CardAction {
private:
  int damage;
  SpriteProxyNode* buster{ nullptr };
  Attachment* busterAttachment{ nullptr };
  Animation busterAnim;
public:
  TwinFangCardAction(std::shared_ptr<Character> actor, int damage);
  ~TwinFangCardAction();

  void Update(double _elapsed) override;
  void OnExecute(std::shared_ptr<Character> user);
  void OnActionEnd() override;
  void OnAnimationEnd() override;
};
