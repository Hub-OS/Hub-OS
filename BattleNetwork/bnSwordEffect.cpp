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

<<<<<<< HEAD
=======
  // Create a callback
  // When animation ends
  // delete this effect
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  auto onEnd = [this]() {
    this->Delete();
  };

  animation << onEnd;

<<<<<<< HEAD
  animation.Update(0, *this);

=======
  // Use the first rect in the frame list
  animation.Update(0, *this);
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}

void SwordEffect::Update(float _elapsed) {
  this->setPosition(this->tile->getPosition());

  animation.Update(_elapsed, *this);
  Entity::Update(_elapsed);
}

SwordEffect::~SwordEffect()
{
}
