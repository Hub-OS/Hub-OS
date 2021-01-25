#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnRingExplosion.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/ring_explosion.animation"

RingExplosion::RingExplosion() : 
  Artifact()
{
  SetLayer(0);
  setTexture(Textures().GetTexture(TextureType::SPELL_RING_EXPLOSION));
  setScale(2.f, 2.f);
  poof = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("EXPLODE");

  auto onEnd = [this]() {
    Delete();
  };

  Audio().Play(AudioType::EXPLODE, AudioPriority::low);

  animation << onEnd;

  animation.Update(0, getSprite());

}

void RingExplosion::OnUpdate(double _elapsed) {
  setPosition(GetTile()->getPosition());

  animation.Update(_elapsed, getSprite());
  Entity::Update(_elapsed);
}

void RingExplosion::OnDelete()
{
  Remove();
}

bool RingExplosion::Move(Direction _direction)
{
  return false;
}

RingExplosion::~RingExplosion()
{
}