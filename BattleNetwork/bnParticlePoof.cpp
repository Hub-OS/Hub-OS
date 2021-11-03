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
  setTexture(Textures().LoadFromFile(TexturePaths::SPELL_POOF));
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
  Entity::drawOffset = -sf::Vector2f{ 0.f, this->GetHeight() };

  animation.Update(_elapsed, getSprite());
}

void ParticlePoof::OnDelete()
{
  Erase();
}

ParticlePoof::~ParticlePoof()
{
}