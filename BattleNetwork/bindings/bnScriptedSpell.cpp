#include "bnScriptedSpell.h"
#include "../bnTile.h"

ScriptedSpell::ScriptedSpell(Team _team) : 
  Spell(_team) {
  setScale(2.f, 2.f);

  shadow = new SpriteProxyNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->Hide(); // default: hidden
  AddNode(shadow);

  animComponent = CreateComponent<AnimationComponent>(this);
}

ScriptedSpell::~ScriptedSpell() {
  delete shadow;
}

bool ScriptedSpell::CanMoveTo(Battle::Tile * next)
{
  return canMoveToCallback? canMoveToCallback(*next) : false;
}

void ScriptedSpell::OnUpdate(double _elapsed) {
  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y - GetHeight());
  //shadow->setPosition(0, +GetHeight()); // counter offset the shadow node
  updateCallback ? updateCallback(this, _elapsed) : (void)0;

}

void ScriptedSpell::OnDelete() {
  deleteCallback ? deleteCallback(this) : (void)0;
  Remove();
}

void ScriptedSpell::Attack(Character* other) {
  other->Hit(GetHitboxProperties());
  attackCallback ? attackCallback(this, other) : (void)0;
}

void ScriptedSpell::OnSpawn(Battle::Tile& spawn)
{
  spawnCallback ? spawnCallback(this, spawn) : (void)0;
}

const float ScriptedSpell::GetHeight() const
{
  return height;
}

void ScriptedSpell::SetHeight(const float height)
{
  this->height = height;
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

AnimationComponent& ScriptedSpell::GetAnimationComponent()
{
  return *animComponent;
}

void ScriptedSpell::SetSlideTimeFrames(unsigned frames)
{
  this->SetSlideTime(time_cast<sf::Time>(::frames(frames)));
}