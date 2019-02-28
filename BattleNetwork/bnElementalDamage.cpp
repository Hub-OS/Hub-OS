#include "bnElementalDamage.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include <Swoosh\Ease.h>
#include <Swoosh\Game.h>

using sf::IntRect;

ElementalDamage::ElementalDamage(Field* field, Team team) : Artifact(field, team), animationComponent(this)
{
  SetLayer(0);
  setTexture(LOAD_TEXTURE(ELEMENT_ALERT));
  setScale(0.f, 0.0f);
  swoosh::game::setOrigin(*this, 0.5, 0.5);
  progress = 0;
}

void ElementalDamage::Update(float _elapsed) {
  progress += _elapsed;

  auto alpha = swoosh::ease::wideParabola(progress, 1.0f, 4.0f);

  if (progress > 1.0f) {
    this->Delete();
  }

  setScale(2.f*alpha, 2.f*alpha);
  setPosition((tile->getPosition().x - 30.0f), (tile->getPosition().y - 30.0f));
  Entity::Update(_elapsed);
}

ElementalDamage::~ElementalDamage()
{
}