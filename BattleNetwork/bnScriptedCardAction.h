#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ScriptedCardAction : public CardAction {
public:
  ScriptedCardAction(Character& owner, int damage) : CardAction(owner, "PLAYER_IDLE") {
    // SCRIPTS.callback(card_name).OnCreate(this);
  }

  ~ScriptedCardAction()
  {
  }

  void OnUpdate(float _elapsed) override
  {
    CardAction::OnUpdate(_elapsed);

    // SCRIPTS.callback(card_name).OnUpdate(this);
  }

  void OnEndAction() override
  {
    // SCRIPTS.callback(card_name).OnEndAction(this);
  }

  void OnExecute() override {

  }
};
