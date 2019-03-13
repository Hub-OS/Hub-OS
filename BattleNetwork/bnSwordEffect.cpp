#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnSwordEffect.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/sword_effect.animation"

SwordEffect::SwordEffect(Field* field) : Artifact(field, Team::UNKNOWN)
{
  SetLayer(0);
  this->setTexture(*TEXTURES.GetTexture(TextureType::SPELL_SWORD));
  this->setScale(2.f, 2.f);

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

void SwordEffect::Update(float _elapsed) {
  this->setPosition(this->tile->getPosition());

  animation.Update(_elapsed, *this);
  Entity::Update(_elapsed);
}

SwordEffect::~SwordEffect()
{
}
