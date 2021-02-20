#include "bnScriptedObstacle.h"

ScriptedObstacle::ScriptedObstacle(Team _team) :
  Obstacle(_team) {
  setScale(2.f, 2.f);

  shadow = new SpriteProxyNode();
  shadow->setTexture(LOAD_TEXTURE(MISC_SHADOW));
  shadow->SetLayer(1);
  shadow->Hide(); // default: hidden
  AddNode(shadow);

  animComponent = CreateComponent<AnimationComponent>(this);
}

ScriptedObstacle::~ScriptedObstacle() {
  delete shadow;
}

bool ScriptedObstacle::CanMoveTo(Battle::Tile * next)
{
  return canMoveToCallback? canMoveToCallback(*next) : false;
}

void ScriptedObstacle::OnUpdate(double _elapsed) {
  setPosition(tile->getPosition().x + Entity::tileOffset.x + ScriptedObstacle::scriptedOffset.x,
    tile->getPosition().y - this->height + Entity::tileOffset.y + ScriptedObstacle::scriptedOffset.y);

  //shadow->setPosition(0, +GetHeight()); // counter offset the shadow node
  ScriptedObstacle& so = *this;
  updateCallback ? updateCallback(so, _elapsed) : (void)0;

}

void ScriptedObstacle::OnDelete() {
  ScriptedObstacle& so = *this;
  deleteCallback ? deleteCallback(so) : (void)0;
  Remove();
}

void ScriptedObstacle::Attack(Character* other) {
  other->Hit(GetHitboxProperties());
  ScriptedObstacle& so = *this;
  attackCallback ? attackCallback(so, *other) : (void)0;
}

void ScriptedObstacle::OnSpawn(Battle::Tile& spawn)
{
  ScriptedObstacle& so = *this;
  spawnCallback ? spawnCallback(so, spawn) : (void)0;
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

Animation& ScriptedObstacle::GetAnimationObject()
{
  return animComponent->GetAnimationObject();
}

void ScriptedObstacle::SetSlideTimeFrames(unsigned frames)
{
  this->SetSlideTime(time_cast<sf::Time>(::frames(frames)));
}

const sf::Vector2f& ScriptedObstacle::GetTileOffset() const
{
  return ScriptedObstacle::scriptedOffset;
}

void ScriptedObstacle::SetTileOffset(float x, float y)
{
  ScriptedObstacle::scriptedOffset = { x, y };
}