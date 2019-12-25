#pragma once
#include "bnChipAction.h"
#include "bnAnimation.h"
#include <SFML/Graphics.hpp>

class SpriteSceneNode;
class Character;
class ScriptedChipAction : public ChipAction {
public:
  ScriptedChipAction(Character * owner, int damage) : ChipAction(owner, "PLAYER_IDLE", nullptr, "Buster") {
    // SCRIPTS.callback(chip_name).onCreate(this);
  }

  ~ScriptedChipAction()
  {
  }

  void OnUpdate(float _elapsed)
  {
    ChipAction::OnUpdate(_elapsed);

    // SCRIPTS.callback(chip_name).onUpdate(this);
  }

  void EndAction()
  {
    GetOwner()->FreeComponentByID(this->GetID());
    // SCRIPTS.callback(chip_name).onDestroy(this);
    delete this;
  }

  void Execute() {

  }
};
