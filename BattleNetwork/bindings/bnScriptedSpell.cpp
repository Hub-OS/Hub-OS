#ifdef BN_MOD_SUPPORT
#include "bnScriptedSpell.h"

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
}

ScriptedSpell::~ScriptedSpell() {
  delete shadow;
}

bool ScriptedSpell::CanMoveTo(Battle::Tile * next)
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

void ScriptedSpell::OnUpdate(double _elapsed) {
  // counter offset the shadow node
  shadow->setPosition(0, Entity::GetCurrJumpHeight() / 2);

  if (updateCallback) {
    auto ss = shared_from_base<ScriptedSpell>();

    try {
      updateCallback(ss, _elapsed);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedSpell::OnDelete() {
  if (deleteCallback) {
    auto ss = shared_from_base<ScriptedSpell>();

    try {
      deleteCallback(ss);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }

  Remove();
}

void ScriptedSpell::OnCollision(const std::shared_ptr<Entity> other)
{
  if (collisionCallback) {
    auto ss = shared_from_base<ScriptedSpell>();

    try {
      collisionCallback(ss, other);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedSpell::Attack(std::shared_ptr<Entity> other) {
  other->Hit(GetHitboxProperties());

  if (attackCallback) {
    auto ss = shared_from_base<ScriptedSpell>();

    try {
      attackCallback(ss, other);
    } catch(std::exception& e) {
      Logger::Log(e.what());
    }
  }
}

void ScriptedSpell::OnSpawn(Battle::Tile& spawn)
{
  if (spawnCallback) {
    auto ss = shared_from_base<ScriptedSpell>();

    try {
      spawnCallback(ss, spawn);
    } catch(std::exception& e) {
      Logger::Log(e.what());
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