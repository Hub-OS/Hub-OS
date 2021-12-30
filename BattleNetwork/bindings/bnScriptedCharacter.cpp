#ifdef BN_MOD_SUPPORT
#include "bnScriptedCharacter.h"
#include "../bnExplodeState.h"
#include "../bnNaviExplodeState.h"
#include "../bnTile.h"
#include "../bnField.h"
#include "../bnTextureResourceManager.h"
#include "../bnAudioResourceManager.h"
#include "../bnGame.h"
#include "../bnDefenseVirusBody.h"
#include "../bnUIComponent.h"

ScriptedCharacter::ScriptedCharacter(Character::Rank rank) :
  AI<ScriptedCharacter>(this), 
  Character(rank)
{
  SetElement(Element::none);
  setScale(2.f, 2.f);
}

ScriptedCharacter::~ScriptedCharacter() {
}

void ScriptedCharacter::Init() {
  Character::Init();

  animation = CreateComponent<AnimationComponent>(weak_from_this());
  weakWrap = WeakWrapper(weak_from_base<ScriptedCharacter>());
}

void ScriptedCharacter::InitFromScript(sol::state& script) {
  if (!HasInit()) {
    Init();
  }

  auto initResult = CallLuaFunction(script, "package_init", weakWrap);

  if (initResult.is_error()) {
    Logger::Log(LogLevel::critical, initResult.error_cstr());
  }

  animation->Refresh();
}

void ScriptedCharacter::OnUpdate(double _elapsed) {
  AI<ScriptedCharacter>::Update(_elapsed);
}

void ScriptedCharacter::SetHeight(const float height) {
  this->height = height;
}

const float ScriptedCharacter::GetHeight() const {
  return height;
}

void ScriptedCharacter::OnDelete() {
  // Explode if health depleted
  if (bossExplosion) {
    ChangeState<NaviExplodeState<ScriptedCharacter>>(numOfExplosions, explosionPlayback);
  }
  else {
    ChangeState<ExplodeState<ScriptedCharacter>>(numOfExplosions, explosionPlayback);
  }

  if (delete_func.valid()) 
  {
    auto result = CallLuaCallback(delete_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }
}

void ScriptedCharacter::OnSpawn(Battle::Tile& start) {
  if (on_spawn_func.valid()) 
  {
    auto result = CallLuaCallback(on_spawn_func, weakWrap, &start);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }
}

void ScriptedCharacter::OnBattleStart() {
  if (battle_start_func.valid()) 
  {
    auto result = CallLuaCallback(battle_start_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }

  InvokeDefaultState();

  Character::OnBattleStart();
}

void ScriptedCharacter::OnBattleStop() {
  Character::OnBattleStop();

  if (battle_end_func.valid()) 
  {
    auto result = CallLuaCallback(battle_end_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }
}

bool ScriptedCharacter::CanMoveTo(Battle::Tile* next) {
  if (auto action = this->CurrentCardAction()) {
    auto res = action->CanMoveTo(next);

    if (res) {
      return *res;
    }
  }

  if (can_move_to_func.valid())
  {
    auto result = CallLuaCallbackExpectingValue<bool>(can_move_to_func, next);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }

    return result.value();
  }

  return Character::CanMoveTo(next);
}

void ScriptedCharacter::RegisterStatusCallback(const Hit::Flags& flag, const StatusCallback& callback)
{
  Character::RegisterStatusCallback(flag, callback);
}

void ScriptedCharacter::ShakeCamera(double power, float duration)
{
  this->EventChannel().Emit(&Camera::ShakeCamera, power, sf::seconds(duration));
}

void ScriptedCharacter::OnCountered()
{
  if (on_countered_func.valid())
  {
    auto result = CallLuaCallback(on_countered_func, weakWrap);

    if (result.is_error()) {
      Logger::Log(LogLevel::critical, result.error_cstr());
    }
  }
}

Animation& ScriptedCharacter::GetAnimationObject() {
  return animation->GetAnimationObject();
}
void ScriptedCharacter::SetExplosionBehavior(int num, double speed, bool isBoss)
{
  numOfExplosions = num;
  explosionPlayback = speed;
  bossExplosion = isBoss;
}

#endif