#pragma once
#include "bnCharacter.h"
#include "bnScriptResourceManager.h"

class ScriptedCharacter : public Character {
public:
  ScriptedCharacter(ScriptedCharacter::Rank rank) : Character(rank) {

  }

  const bool OnHit(const Hit::Properties props) final {
    // SCRIPTS.callback(character_ID).OnHit(props);
    return false;
  }

  const float GetHeight() const final {
    // SCRIPTS.callback(character_ID).GetHeight(props);
    return 0;
  }

  void OnDelete() override final {

  }

  void OnUpdate(float elapsed) final {

  }
};