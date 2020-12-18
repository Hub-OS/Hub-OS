#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class TwinFangCardAction : public CardAction {
private:
  int damage;
public:
  TwinFangCardAction(Character& owner, int damage);
  ~TwinFangCardAction();

  void OnExecute();
  void OnEndAction() override;
  void OnUpdate(double _elapsed) override;
  void OnAnimationEnd() override;
};
