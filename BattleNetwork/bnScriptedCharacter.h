#pragma once
#include "bnCharacter.h"
#include "bnScriptResourceManager.h"

class ScriptedCharacter : public Character {
public:
  ScriptedCharacter(ScriptedCharacter::Rank rank) : Character(rank) {

  }

  const float GetHeight() const final {
    // SCRIPTS.callback(character_ID).GetHeight(props);
    return 0;
  }

  void OnDelete() override final {

  }

  void OnUpdate(double elapsed) final {

  }
};