#include "bnElementalDamage.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnField.h"
#include "bnTile.h"

#include <Swoosh/Ease.h>
#include <Swoosh/Game.h>

using sf::IntRect;

ElementalDamage::ElementalDamage(Field* field) : Artifact(field), animationComponent(this)
{
  SetLayer(0);
  setTexture(LOAD_TEXTURE(ELEMENT_ALERT));
  setScale(0.f, 0.0f);
  swoosh::game::setOrigin(*this, 0.5, 0.5);
  progress = 0;
}

void ElementalDamage::OnUpdate(float _elapsed) {
  progress += _elapsed;

  auto alpha = swoosh::ease::wideParabola(progress, 0.5f, 4.0f);

  if (progress > 1.0f) {
    this->Delete();
  }

  setScale(2.f*alpha, 2.f*alpha);
  setPosition((GetTile()->getPosition().x - 30.0f), (GetTile()->getPosition().y - 30.0f));
}

ElementalDamage::~ElementalDamage()
{
}
