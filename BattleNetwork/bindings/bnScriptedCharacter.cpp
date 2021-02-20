#include "bnScriptedCharacter.h"
#include "../bnExplodeState.h"
#include "../bnTile.h"
#include "../bnField.h"
#include "../bnTextureResourceManager.h"
#include "../bnAudioResourceManager.h"
#include "../bnGame.h"
#include "../bnDefenseVirusBody.h"

ScriptedCharacter::ScriptedCharacter(sol::state& script) : 
  script(script),
  AI<ScriptedCharacter>(this), 
  Character() {
  SetElement(Element::none);
  SetTeam(Team::blue);
  setScale(2.f, 2.f);

  //Components setup and load
  animation = CreateComponent<AnimationComponent>(this);

  script["battle_init"](this);

  animation->Refresh();
}

ScriptedCharacter::~ScriptedCharacter() {
}

void ScriptedCharacter::SetRank(Character::Rank rank)
{
  this->rank = rank;
}

void ScriptedCharacter::OnUpdate(double _elapsed) {
  setPosition(tile->getPosition().x + Entity::tileOffset.x + scriptedOffset.x,
    tile->getPosition().y + Entity::tileOffset.y + scriptedOffset.y);

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
  int num_of_explosions = script["num_of_explosions"]();
  ChangeState<ExplodeState<ScriptedCharacter>>(num_of_explosions);
}

void ScriptedCharacter::OnSpawn(Battle::Tile& start) {
  script["on_spawn"](this, start);
}

void ScriptedCharacter::OnBattleStart() {
  script["on_battle_start"]();
}

void ScriptedCharacter::OnBattleStop() {
  script["on_battle_stop"]();
}

bool ScriptedCharacter::CanMoveTo(Battle::Tile * next) {
  return script["can_move_to"](*next);
}

void ScriptedCharacter::SetSlideTimeFrames(unsigned frames)
{
  this->SetSlideTime(time_cast<sf::Time>(::frames(frames)));
}

const sf::Vector2f& ScriptedCharacter::GetTileOffset() const
{
  return ScriptedCharacter::scriptedOffset;
}

void ScriptedCharacter::SetTileOffset(float x, float y)
{
  ScriptedCharacter::scriptedOffset = { x, y };
}

void ScriptedCharacter::ShakeCamera(double power, float duration)
{
  this->EventBus().Emit(&Camera::ShakeCamera, power, sf::seconds(duration));
}

Animation& ScriptedCharacter::GetAnimationObject() {
  return animation->GetAnimationObject();
}
