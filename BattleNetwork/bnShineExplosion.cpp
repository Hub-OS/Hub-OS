#include "bnShineExplosion.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"

using sf::IntRect;

ShineExplosion::ShineExplosion(Field* _field, Team _team) : animationComponent(this)
{
  SetLayer(0);
  field = _field;
  team = _team;
  setTexture(LOAD_TEXTURE(MOB_BOSS_SHINE));
  setScale(2.f, 2.f);
  animationComponent.Setup("resources/mobs/boss_shine.animation");
  animationComponent.Reload();
  animationComponent.SetAnimation("SHINE", Animate::Mode::Loop);
  animationComponent.Update(0.0f);
}

void ShineExplosion::Update(float _elapsed) {
  animationComponent.Update(_elapsed);
  setPosition(tile->getPosition().x, tile->getPosition().y - 30.f);
  Entity::Update(_elapsed);
}

vector<Drawable*> ShineExplosion::GetMiscComponents() {
  vector<Drawable*> drawables = vector<Drawable*>();
  return drawables;
}

ShineExplosion::~ShineExplosion()
{
}