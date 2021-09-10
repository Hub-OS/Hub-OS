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
  return canMoveToCallback? canMoveToCallback(*next) : false;
}

void ScriptedObstacle::OnCollision(const std::shared_ptr<Entity> other)
{
  auto so = shared_from_base<ScriptedObstacle>();
  collisionCallback ? collisionCallback(so, other) : (void)0;
}

void ScriptedObstacle::OnUpdate(double _elapsed) {
  // counter offset the shadow node
  shadow->setPosition(0, Entity::GetCurrJumpHeight() / 2);
  auto so = shared_from_base<ScriptedObstacle>();
  updateCallback ? updateCallback(so, _elapsed) : (void)0;

}

void ScriptedObstacle::OnDelete() {
  auto so = shared_from_base<ScriptedObstacle>();
  deleteCallback ? deleteCallback(so) : (void)0;
  Remove();
}

void ScriptedObstacle::Attack(std::shared_ptr<Entity> other) {
  other->Hit(GetHitboxProperties());
  auto so = shared_from_base<ScriptedObstacle>();
  attackCallback ? attackCallback(so, other) : (void)0;
}

void ScriptedObstacle::OnSpawn(Battle::Tile& spawn)
{
  auto so = shared_from_base<ScriptedObstacle>();
  spawnCallback ? spawnCallback(so, spawn) : (void)0;

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