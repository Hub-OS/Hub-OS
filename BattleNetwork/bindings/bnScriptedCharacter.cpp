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
#include "../bnSolHelpers.h"

ScriptedCharacter::ScriptedCharacter(sol::state& script, Character::Rank rank) :
  script(script),
  AI<ScriptedCharacter>(this), 
  Character(rank)
{
  SetElement(Element::none);
  SetTeam(Team::blue);
  setScale(2.f, 2.f);
}

ScriptedCharacter::~ScriptedCharacter() {
}

void ScriptedCharacter::Init() {
  Character::Init();

  animation = CreateComponent<AnimationComponent>(weak_from_this());

  auto character = WeakWrapper(weak_from_base<ScriptedCharacter>());
  auto initResult = CallLuaFunction(script, "package_init", character);

  if (initResult.is_error()) {
    Logger::Log(initResult.error_cstr());
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

  if (deleteCallback) {
    try {
      auto character = WeakWrapper(weak_from_base<ScriptedCharacter>());
      deleteCallback(character); 
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedCharacter::OnSpawn(Battle::Tile& start) {
  if (spawnCallback) {
    try {
      auto character = WeakWrapper(weak_from_base<ScriptedCharacter>());
      spawnCallback(character, start);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedCharacter::OnBattleStart() {
  if (onBattleStartCallback) {
    try {
      auto character = WeakWrapper(weak_from_base<ScriptedCharacter>());
      onBattleStartCallback(character);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedCharacter::OnBattleStop() {
  Character::OnBattleStop();

  if (onBattleEndCallback) {
    try {
      auto character = WeakWrapper(weak_from_base<ScriptedCharacter>());
      onBattleEndCallback(character);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

bool ScriptedCharacter::CanMoveTo(Battle::Tile * next) {
  if (canMoveToCallback) {
    try {
      return canMoveToCallback(*next);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
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

Animation& ScriptedCharacter::GetAnimationObject() {
  return animation->GetAnimationObject();
}
void ScriptedCharacter::SetExplosionBehavior(int num, double speed, bool isBoss)
{
  numOfExplosions = num;
  explosionPlayback = speed;
  bossExplosion = isBoss;
}

void ScriptedCharacter::SimpleCardActionEvent(std::shared_ptr<ScriptedCardAction> action, ActionOrder order)
{
  Character::AddAction(CardEvent{ action }, order);
}

void ScriptedCharacter::SimpleCardActionEvent(std::shared_ptr<CardAction> action, ActionOrder order)
{
  Character::AddAction(CardEvent{ action }, order);
}
#endif