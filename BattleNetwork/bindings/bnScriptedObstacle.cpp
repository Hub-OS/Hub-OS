#ifdef BN_MOD_SUPPORT
#include "bnScriptedObstacle.h"
#include "../bnDefenseObstacleBody.h"

ScriptedObstacle::ScriptedObstacle(Team _team) :
  Obstacle(_team) {
  setScale(2.f, 2.f);

  shadow = new SpriteProxyNode();
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
}

ScriptedObstacle::~ScriptedObstacle() {
  delete shadow;
}

bool ScriptedObstacle::CanMoveTo(Battle::Tile * next)
{
  if (canMoveToCallback) {
    try {
      return canMoveToCallback(*next);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }

  return false;
}

void ScriptedObstacle::OnCollision(const std::shared_ptr<Entity> other)
{
  if (!collisionCallback) {
    return;
  }

  auto so = shared_from_base<ScriptedObstacle>();

  try {
    collisionCallback(so, other);
  } catch(std::exception& e) {
    Logger::Log(e.what());
  }
}

void ScriptedObstacle::OnUpdate(double _elapsed) {
  // counter offset the shadow node
  shadow->setPosition(0, Entity::GetCurrJumpHeight() / 2);

  if (updateCallback) {
    auto so = shared_from_base<ScriptedObstacle>();

    try {
      updateCallback(so, _elapsed);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedObstacle::OnDelete() {
  if (deleteCallback) {
    auto so = shared_from_base<ScriptedObstacle>();

    try {
      deleteCallback(so);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }

  Remove();
}

void ScriptedObstacle::Attack(std::shared_ptr<Entity> other) {
  other->Hit(GetHitboxProperties());

  if (attackCallback) {
    auto so = shared_from_base<ScriptedObstacle>();

    try {
      attackCallback(so, other);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedObstacle::OnSpawn(Battle::Tile& spawn)
{
  if (spawnCallback) {
    auto so = shared_from_base<ScriptedObstacle>();

    try {
      spawnCallback(so, spawn);
    } catch(std::exception& e) {
      Logger::Log(e.what());
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