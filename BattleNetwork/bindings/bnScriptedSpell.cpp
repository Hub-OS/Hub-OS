#ifdef BN_MOD_SUPPORT
#include "bnScriptedSpell.h"
#include "../bnSolHelpers.h"
#include "../bnTile.h"

ScriptedSpell::ScriptedSpell(Team team) : Spell(team) {
  setScale(2.f, 2.f);

  shadow = new SpriteProxyNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->Hide(); // default: hidden
  shadow->setOrigin(shadow->getSprite().getLocalBounds().width * 0.5, shadow->getSprite().getLocalBounds().height * 0.5);
  AddNode(shadow);
}

void ScriptedSpell::Init() {
  Spell::Init();
  animComponent = CreateComponent<AnimationComponent>(weak_from_base<Entity>());
  weakWrap = WeakWrapper(weak_from_base<ScriptedSpell>());
}

ScriptedSpell::~ScriptedSpell() {
  delete shadow;
}

bool ScriptedSpell::CanMoveTo(Battle::Tile * next)
{
  if (entries["can_move_to_func"].valid()) 
  {
    auto result = CallLuaFunctionExpectingValue<bool>(entries, "can_move_to_func", next);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }

    return result.value();
  }

  return false;
}

void ScriptedSpell::OnUpdate(double _elapsed) {
  // counter offset the shadow node
  shadow->setPosition(0, Entity::GetCurrJumpHeight() / 2);

  if (entries["update_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "update_func", weakWrap, _elapsed);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedSpell::OnDelete() {
  if (entries["delete_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "delete_func", weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }

  Remove();
}

void ScriptedSpell::OnCollision(const std::shared_ptr<Entity> other)
{
  if (entries["collision_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "collision_func", weakWrap, WeakWrapper(other));

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedSpell::Attack(std::shared_ptr<Entity> other) {
  other->Hit(GetHitboxProperties());

  if (entries["attack_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "attack_func", weakWrap, WeakWrapper(other));

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedSpell::OnSpawn(Battle::Tile& spawn)
{
  if (entries["on_spawn_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "on_spawn_func", weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }

  if (GetTeam() == Team::blue && flip) {
    setScale(-2.f, 2.f);
  }
}

const float ScriptedSpell::GetHeight() const
{
  return height;
}

void ScriptedSpell::SetHeight(const float height)
{
  this->height = height;
  Entity::drawOffset.y = -this->height;
}

void ScriptedSpell::ShowShadow(const bool show)
{
  if (!show) {
    this->shadow->Hide();
  }
  else {
    this->shadow->Reveal();
  }
}

void ScriptedSpell::SetAnimation(const std::string& path)
{
  animComponent->SetPath(path);
  animComponent->Load();
}

Animation& ScriptedSpell::GetAnimationObject()
{
  return animComponent->GetAnimationObject();
}

void ScriptedSpell::ShakeCamera(double power, float duration)
{
  this->EventChannel().Emit(&Camera::ShakeCamera, power, sf::seconds(duration));
}

void ScriptedSpell::NeverFlip (bool enabled)
{
  flip = !enabled;
}

#endif