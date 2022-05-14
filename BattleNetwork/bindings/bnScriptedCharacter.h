#pragma once
#ifdef ONB_MOD_SUPPORT

#include <sol/sol.hpp>
#include "dynamic_object.h"
#include "../bnCharacter.h"
#include "../bnAI.h"
#include "../bnSolHelpers.h"
#include "bnScriptedCardAction.h"
#include "bnWeakWrapper.h"

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

  float height{};
  std::shared_ptr<AnimationComponent> animation{ nullptr };
  bool bossExplosion{ false };
  double explosionPlayback{ 1.0 };
  int numOfExplosions{ 2 };
  WeakWrapper<ScriptedCharacter> weakWrap;
public:
  using DefaultState = ScriptedCharacterState;

  ScriptedCharacter(Character::Rank rank);
  ~ScriptedCharacter();
  void Init();
  void InitFromScript(sol::state& script);
  void OnSpawn(Battle::Tile& start) override;
  void OnBattleStart() override;
  void OnBattleStop() override;
  void OnUpdate(double _elapsed) override ;
  const float GetHeight() const override;
  void SetHeight(const float height);
  void OnDelete() override;
  bool CanMoveTo(Battle::Tile *next) override;
  void RegisterStatusCallback(const Hit::Flags& flag, const StatusCallback& callback);
  void ShakeCamera(double power, float duration);
  void OnCountered() override;
  Animation& GetAnimationObject();
  void SetExplosionBehavior(int num, double speed, bool isBoss);

  sol::object on_update_func;
  sol::object on_delete_func;
  sol::object on_spawn_func;
  sol::object on_battle_start_func;
  sol::object on_battle_end_func;
  sol::object on_countered_func;
  sol::object on_intro_func;
  sol::object can_move_to_func;
};

class ScriptedCharacterState : public AIState<ScriptedCharacter> {
public:
  void OnEnter(ScriptedCharacter& s) override {
  }

  void OnUpdate(double elapsed, ScriptedCharacter& s) override {
    if (s.on_update_func.valid())
    {
      auto result = CallLuaCallback(s.on_update_func, s.weakWrap, elapsed);

      if (result.is_error()) {
        Logger::Log(LogLevel::critical, result.error_cstr());
      }
    }
  }

  void OnLeave(ScriptedCharacter& s) override {

  }
};

using FinishNotifier = std::function<void()>;

class ScriptedIntroState : public AIState<ScriptedCharacter> {
private:
  sol::state& script;
  std::string targetState;
  FinishNotifier finishNotifier;
  long long time{};

  void RunIntroFunc(ScriptedCharacter& context, double elapsed) {
    if (context.on_intro_func.valid())
    {
      auto result = CallLuaCallback(context.on_intro_func, context.weakWrap, targetState, finishNotifier, elapsed);

      if (result.is_error()) {
        Logger::Log(LogLevel::critical, result.error_cstr());
      }
    }

    constexpr long long maxWaitTime = 20 * 1'000;
    if (CurrentTime::AsMilli() - time >= maxWaitTime) {
      finishNotifier();
    }
  }

public:
  ScriptedIntroState(FinishNotifier finishNotifier, sol::state& script, std::string targetState) :
    script(script),
    targetState(targetState),
    finishNotifier(finishNotifier) 
  {}

  void OnEnter(ScriptedCharacter& context) override {
    time = CurrentTime::AsMilli();
    RunIntroFunc(context, 0);
  }

  void OnUpdate(double _elapsed, ScriptedCharacter& context) override {
    RunIntroFunc(context, _elapsed);
  }

  void OnLeave(ScriptedCharacter& context) override {

  }
};

#endif