#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class RecoverCardAction : public CardAction {
  int heal;

public:
  RecoverCardAction(Character* owner, int heal);
  ~RecoverCardAction();
  void OnUpdate(float _elapsed);
  void OnAnimationEnd() override;
  void EndAction();
  void Execute();
};
