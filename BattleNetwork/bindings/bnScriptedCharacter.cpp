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

ScriptedCharacter::ScriptedCharacter(sol::state& script, Character::Rank rank) :
  script(script),
  AI<ScriptedCharacter>(this), 
  Character(rank) {
  SetElement(Element::none);
  SetTeam(Team::blue);
  setScale(2.f, 2.f);

  //Components setup and load
  animation = CreateComponent<AnimationComponent>(this);

  script["package_init"](this);

  animation->Refresh();
}

ScriptedCharacter::~ScriptedCharacter() {
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
    deleteCallback(*this);
  }
}

void ScriptedCharacter::OnSpawn(Battle::Tile& start) {
  if (spawnCallback) {
    spawnCallback(*this, start);
  }
}

void ScriptedCharacter::OnBattleStart() {
  if (onBattleStartCallback) {
    onBattleStartCallback(*this);
  }
}

void ScriptedCharacter::OnBattleStop() {
  Character::OnBattleStop();

  if (onBattleEndCallback) {
    onBattleEndCallback(*this);
  }
}

bool ScriptedCharacter::CanMoveTo(Battle::Tile * next) {
  if (canMoveToCallback) {
    return canMoveToCallback(*next);
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

void ScriptedCharacter::SimpleCardActionEvent(std::unique_ptr<ScriptedCardAction>& action, ActionOrder order)
{
  // getting around sol limitations:
  // using unique_ptr to allow sol to manage memory
  // but we need to share this with the subsystems...
  std::unique_ptr uniqueAction = std::move(action);
  std::shared_ptr<CardAction> sharedAction;
  sharedAction.reset(uniqueAction.release());
  Character::AddAction(CardEvent{ sharedAction }, order);
}

void ScriptedCharacter::SimpleCardActionEvent(std::unique_ptr<CardAction>& action, ActionOrder order)
{
  // see previous function for explanation
  std::unique_ptr uniqueAction = std::move(action);
  std::shared_ptr<CardAction> sharedAction;
  sharedAction.reset(uniqueAction.release());
  Character::AddAction(CardEvent{ sharedAction }, order);
}
#endif