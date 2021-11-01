#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnMobMoveEffect.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/mob_move.animation"

MobMoveEffect::MobMoveEffect() :
  Artifact()
{
  SetLayer(-1);
  setTexture(Textures().LoadFromFile(TexturePaths::MOB_MOVE));
  setScale(2.f, 2.f);
  move = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

  auto onEnd = [this]() {
    Delete();
  };

  animation << onEnd;

  animation.Update(0, getSprite());

}

void MobMoveEffect::OnUpdate(double _elapsed) {
  Entity::drawOffset = offset;

  animation.Update(_elapsed, getSprite());
}

void MobMoveEffect::OnDelete()
{
  Erase();
}

void MobMoveEffect::SetOffset(const sf::Vector2f& offset)
{
  this->offset = offset;
}

MobMoveEffect::~MobMoveEffect()
{
}