#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnParticlePoof.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/poof.animation"

ParticlePoof::ParticlePoof() : Artifact(nullptr)
{
  SetLayer(0);
  this->setTexture(TEXTURES.GetTexture(TextureType::SPELL_POOF));
  this->setScale(2.f, 2.f);
  poof = getSprite();

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

  auto onEnd = [this]() {
    this->Delete();
  };

  animation << onEnd;

  animation.Update(0, this->getSprite());

}

void ParticlePoof::OnUpdate(float _elapsed) {
  this->setPosition(this->GetTile()->getPosition());

  animation.Update(_elapsed, this->getSprite());
  Entity::Update(_elapsed);
}

ParticlePoof::~ParticlePoof()
{
}