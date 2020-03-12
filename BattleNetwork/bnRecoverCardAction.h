#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class RecoverCardAction : public CardAction {
  int heal;

public:
  RecoverCardAction(Character* owner, int heal);
  ~RecoverCardAction();
  void OnUpdate(float _elapsed);
  void EndAction();
  void Execute();
};
