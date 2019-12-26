#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnParticleHeal.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/spell_heal.animation"

ParticleHeal::ParticleHeal() : Artifact(nullptr)
{
  SetLayer(0);
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_HEAL));
  this->setScale(2.f, 2.f);
  fx = (sf::Sprite)*this;

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

  auto onEnd = [this]() {
    this->Delete();
  };

  animation << onEnd;

  animation.Update(0, *this);

}

void ParticleHeal::OnUpdate(float _elapsed) {
  this->setPosition(this->GetTile()->getPosition());

  animation.Update(_elapsed, *this);
  Entity::Update(_elapsed);
}

ParticleHeal::~ParticleHeal()
{
}