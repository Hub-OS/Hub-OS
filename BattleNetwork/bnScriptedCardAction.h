#pragma once
#include "bnCardAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteProxyNode;
class Character;
class ScriptedCardAction : public CardAction {
public:
  ScriptedCardAction(Character * owner, int damage) : CardAction(owner, "PLAYER_IDLE", nullptr, "Buster") {
    // SCRIPTS.callback(card_name).onCreate(this);
  }

  ~ScriptedCardAction()
  {
  }

  void OnUpdate(float _elapsed)
  {
    CardAction::OnUpdate(_elapsed);

    // SCRIPTS.callback(card_name).onUpdate(this);
  }

  void EndAction()
  {
    GetOwner()->FreeComponentByID(this->GetID());
    // SCRIPTS.callback(card_name).onDestroy(this);
    delete this;
  }

  void Execute() {

  }
};
