#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnParticlePoof.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/poof.animation"

ParticlePoof::ParticlePoof() : 
  Artifact()
{
  SetLayer(0);
  setTexture(Textures().GetTexture(TextureType::SPELL_POOF));
  setScale(2.f, 2.f);
  poof = getSprite();

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

void ParticlePoof::OnUpdate(double _elapsed) {
  setPosition(GetTile()->getPosition());

  animation.Update(_elapsed, getSprite());
  Entity::Update(_elapsed);
}

void ParticlePoof::OnDelete()
{
  Remove();
}

ParticlePoof::~ParticlePoof()
{
}