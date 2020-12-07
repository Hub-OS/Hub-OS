#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnMobMoveEffect.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/mobs/mob_move.animation"

MobMoveEffect::MobMoveEffect(Field* field) : Artifact(field)
{
  SetLayer(-1);
  setTexture(Textures().GetTexture(TextureType::MOB_MOVE));
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

void MobMoveEffect::OnUpdate(float _elapsed) {
  setPosition(GetTile()->getPosition() + offset);

  animation.Update(_elapsed, getSprite());
}

void MobMoveEffect::OnDelete()
{
  Remove();
}

bool MobMoveEffect::Move(Direction _direction)
{
  return false;
}

void MobMoveEffect::SetOffset(const sf::Vector2f& offset)
{
  this->offset = offset;
}

MobMoveEffect::~MobMoveEffect()
{
}