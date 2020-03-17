#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnRingExplosion.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/ring_explosion.animation"

RingExplosion::RingExplosion(Field* field) : Artifact(field)
{
  SetLayer(0);
  this->setTexture(TEXTURES.GetTexture(TextureType::SPELL_RING_EXPLOSION));
  this->setScale(2.f, 2.f);
  poof = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("EXPLODE");

  auto onEnd = [this]() {
    this->Delete();
  };

  AUDIO.Play(AudioType::EXPLODE, AudioPriority::LOW);

  animation << onEnd;

  animation.Update(0, *this);

}

void RingExplosion::OnUpdate(float _elapsed) {
  this->setPosition(this->GetTile()->getPosition());

  animation.Update(_elapsed, *this);
  Entity::Update(_elapsed);
}

RingExplosion::~RingExplosion()
{
}