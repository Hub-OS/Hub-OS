#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnSwordEffect.h"

using sf::IntRect;

#define RESOURCE_PATH "resources/spells/sword_effect.animation"

SwordEffect::SwordEffect(Field* field) : Artifact(field)
{
  SetLayer(0);
  this->setTexture(TEXTURES.GetTexture(TextureType::SPELL_SWORD));
  this->setScale(2.f, 2.f);

  //Components setup and load
  animation = Animation(RESOURCE_PATH);
  animation.Reload();

  animation.SetAnimation("DEFAULT");

  // Create a callback
  // When animation ends
  // delete this effect
  auto onEnd = [this]() {
    this->Delete();
  };

  animation << onEnd;

  // Use the first rect in the frame list
  animation.Update(0, *this);
}

void SwordEffect::OnUpdate(float _elapsed) {
  this->setPosition(this->GetTile()->getPosition());

  animation.Update(_elapsed, *this);
}

SwordEffect::~SwordEffect()
{
}
