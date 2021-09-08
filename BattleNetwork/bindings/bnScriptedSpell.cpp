#ifdef BN_MOD_SUPPORT
#include "bnScriptedSpell.h"

ScriptedSpell::ScriptedSpell(Team _team) : 
  Spell(_team) {
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
  return canMoveToCallback? canMoveToCallback(*next) : false;
}

void ScriptedSpell::OnUpdate(double _elapsed) {
  // counter offset the shadow node
  shadow->setPosition(0, Entity::GetCurrJumpHeight() / 2);
  auto ss = shared_from_base<ScriptedSpell>();
  updateCallback ? updateCallback(ss, _elapsed) : (void)0;

}

void ScriptedSpell::OnDelete() {
  auto ss = shared_from_base<ScriptedSpell>();
  deleteCallback ? deleteCallback(ss) : (void)0;
  Remove();
}

void ScriptedSpell::OnCollision(const std::shared_ptr<Character> other)
{
  auto ss = shared_from_base<ScriptedSpell>();
  collisionCallback ? collisionCallback(ss, other) : (void)0;
}

void ScriptedSpell::Attack(std::shared_ptr<Character> other) {
  other->Hit(GetHitboxProperties());
  auto ss = shared_from_base<ScriptedSpell>();
  attackCallback ? attackCallback(ss, other) : (void)0;
}

void ScriptedSpell::OnSpawn(Battle::Tile& spawn)
{
  auto ss = shared_from_base<ScriptedSpell>();
  spawnCallback ? spawnCallback(ss, spawn) : (void)0;

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