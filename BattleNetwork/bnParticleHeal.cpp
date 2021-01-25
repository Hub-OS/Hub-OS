#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnParticleHeal.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/spell_heal.animation"

ParticleHeal::ParticleHeal() : Artifact()
{
  SetLayer(0);
  setTexture(Textures().GetTexture(TextureType::SPELL_HEAL));
  setScale(2.f, 2.f);
  fx = getSprite();

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

void ParticleHeal::OnUpdate(double _elapsed) {
  setPosition(GetTile()->getPosition());

  animation.Update(_elapsed, getSprite());
  Entity::Update(_elapsed);
}

void ParticleHeal::OnDelete()
{
  Remove();
}

bool ParticleHeal::Move(Direction _direction)
{
  return false;
}

ParticleHeal::~ParticleHeal()
{
}