#include "bnElementalDamage.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"

#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

using sf::IntRect;

ElementalDamage::ElementalDamage() : 
  Artifact(), 
  animationComponent(shared_from_this())
{
  SetLayer(0);
  setTexture(LOAD_TEXTURE(ELEMENT_ALERT));
  setScale(0.f, 0.0f);
  swoosh::game::setOrigin(getSprite(), 0.5, 0.5);
  progress = 0;
}

void ElementalDamage::OnUpdate(double _elapsed) {
  progress += _elapsed;

  float alpha = swoosh::ease::wideParabola(static_cast<float>(progress), 0.5f, 4.0f);

  if (progress > 1.0) {
    Delete();
  }

  setScale(2.f*alpha, 2.f*alpha);
  Entity::drawOffset = {-30.0f, -30.0f };
}

void ElementalDamage::OnDelete()
{
}

ElementalDamage::~ElementalDamage()
{
}
