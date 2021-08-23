#pragma once
#ifdef BN_MOD_SUPPORT

#include <sol/sol.hpp>
#include "dynamic_object.h"
#include "../bnCharacter.h"
#include "../bnAI.h"
#include "bnScriptedCardAction.h"

class AnimationComponent;
class ScriptedCharacterState;
class ScriptedIntroState;

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
  bool bossExplosion{ false };
  double explosionPlayback{ 1.0 };
  int numOfExplosions{ 2 };
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
  void RegisterStatusCallback(const Hit::Flags& flag, const StatusCallback& callback);
  void ShakeCamera(double power, float duration);
  Animation& GetAnimationObject();
  void SetExplosionBehavior(int num, double speed, bool isBoss);
  void SimpleCardActionEvent(std::unique_ptr<ScriptedCardAction>& action, ActionOrder order);
  void SimpleCardActionEvent(std::unique_ptr<CardAction>& action, ActionOrder order);

  std::function<void(ScriptedCharacter&, Battle::Tile&)> spawnCallback;
  std::function<bool(Battle::Tile&)> canMoveToCallback;
  std::function<void(ScriptedCharacter&)> deleteCallback;
  std::function<void(ScriptedCharacter&)> onBattleStartCallback;
  std::function<void(ScriptedCharacter&)> onBattleEndCallback;
  std::function<void(ScriptedCharacter&, double)> updateCallback;
};

class ScriptedCharacterState : public AIState<ScriptedCharacter> {
public:
  void OnEnter(ScriptedCharacter& s) override {
  }

  void OnUpdate(double elapsed, ScriptedCharacter& s) override {
    s.updateCallback(s, elapsed);
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