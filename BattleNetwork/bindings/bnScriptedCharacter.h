#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "dynamic_object.h"
#include "../bnCharacter.h"
#include "../bnAI.h"

/**
 * @class ScriptedCharacter
 * @author mav
 * @date 02/10/21
 * @brief Character reads from lua files for behavior and settings
 */
class ScriptedCharacter final : public Character, public AI<ScriptedCharacter>, public dynamic_object {
  friend class ScriptedCharacterState;
  friend class ScriptedIntroState;
  sol::state& script;
  float height{};
  AnimationComponent* animation{ nullptr };
  sf::Vector2f scriptedOffset{};
public:
  using DefaultState = ScriptedCharacterState;

  ScriptedCharacter(sol::state& script);
  ~ScriptedCharacter();
  void SetRank(Character::Rank rank);
  void OnSpawn(Battle::Tile& start) override;
  void OnBattleStart() override;
  void OnBattleStop() override;
  void OnUpdate(double _elapsed) override ;
  const float GetHeight() const override;
  void SetHeight(const float height);
  void OnDelete() override;
  bool CanMoveTo(Battle::Tile * next) override;
  const sf::Vector2f& GetTileOffset() const;
  void SetTileOffset(float x, float y);
  Battle::Tile* GetCurrentTile() const;

  // duration in seconds
  void ShakeCamera(double power, float duration);

  Animation& GetAnimationObject();
};

class ScriptedCharacterState : public AIState<ScriptedCharacter> {
public:
  void OnEnter(ScriptedCharacter& s) override {
  }

  void OnUpdate(double elapsed, ScriptedCharacter& s) override {
    s.script["on_update"](s, elapsed);
  }

  void OnLeave(ScriptedCharacter& s) override {

  }
};

class ScriptedIntroState : public AIState<ScriptedCharacter> {
  void OnEnter(ScriptedCharacter& context) override {

  }

  void OnUpdate(double _elapsed, ScriptedCharacter& context) override {

  }

  void OnLeave(ScriptedCharacter& context) override {

  }
};

#endif 