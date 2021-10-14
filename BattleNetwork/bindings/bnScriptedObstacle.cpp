#ifdef BN_MOD_SUPPORT
#include "bnScriptedObstacle.h"
#include "../bnDefenseObstacleBody.h"
#include "../bnSolHelpers.h"

ScriptedObstacle::ScriptedObstacle(Team _team) :
  Obstacle(_team) {
  setScale(2.f, 2.f);

  shadow = std::make_shared<SpriteProxyNode>();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->Hide(); // default: hidden
  shadow->setOrigin(shadow->getSprite().getLocalBounds().width * 0.5, shadow->getSprite().getLocalBounds().height * 0.5);
  AddNode(shadow);
}

void ScriptedObstacle::Init() {
  Obstacle::Init();

  animComponent = CreateComponent<AnimationComponent>(weak_from_this());
  animComponent->Load();
  animComponent->Refresh();

  obstacleBody = std::make_shared<DefenseObstacleBody>();
  this->AddDefenseRule(obstacleBody);

  weakWrap = WeakWrapper(weak_from_base<ScriptedObstacle>());
}

ScriptedObstacle::~ScriptedObstacle() {
}

bool ScriptedObstacle::CanMoveTo(Battle::Tile * next)
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

void ScriptedObstacle::OnCollision(const std::shared_ptr<Entity> other)
{
  if (entries["collision_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "collision_func", weakWrap, WeakWrapper(other));

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedObstacle::OnUpdate(double _elapsed) {
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

void ScriptedObstacle::OnDelete() {
  if (entries["delete_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "delete_func", weakWrap);

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }

  Remove();
}

void ScriptedObstacle::Attack(std::shared_ptr<Entity> other) {
  other->Hit(GetHitboxProperties());

  if (entries["attack_func"].valid()) 
  {
    auto result = CallLuaFunction(entries, "attack_func", weakWrap, WeakWrapper(other));

    if (result.is_error()) {
      Logger::Log(result.error_cstr());
    }
  }
}

void ScriptedObstacle::OnSpawn(Battle::Tile& spawn)
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

const float ScriptedObstacle::GetHeight() const
{
  return height;
}

void ScriptedObstacle::SetHeight(const float height)
{
  this->height = height;
}

void ScriptedObstacle::ShowShadow(const bool show)
{
  if (!show) {
    this->shadow->Hide();
  }
  else {
    this->shadow->Reveal();
  }
}

void ScriptedObstacle::SetAnimation(const std::string& path)
{
  animComponent->SetPath(path);
  animComponent->Load();
}

Animation& ScriptedObstacle::GetAnimationObject()
{
  return animComponent->GetAnimationObject();
}

Battle::Tile* ScriptedObstacle::GetCurrentTile() const
{
  return GetTile();
}

void ScriptedObstacle::ShakeCamera(double power, float duration)
{
}
void ScriptedObstacle::NeverFlip(bool enabled)
{
  flip = !enabled;
}
#endif