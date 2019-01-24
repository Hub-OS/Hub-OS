#include "bnEntity.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnChargeComponent.h"

ChargeComponent::ChargeComponent(Entity* _entity) {
  entity = _entity;
  charging = false;
  chargeCounter = 0.0f;
  chargeSprite = sf::Sprite(LOAD_TEXTURE(SPELL_BUSTER_CHARGE));
  chargeSprite.setScale(2.0f, 2.0f);

  animation = Animation("resources/spells/spell_buster_charge.animation");

  isCharged = isPartiallyCharged = false;
}

ChargeComponent::~ChargeComponent() {
}

void ChargeComponent::Update(float _elapsed) {
  if (!charging) {
    chargeCounter = 0.0f;
    chargeSprite.setScale(0.0f, 0.0f);
    isCharged = false;
    isPartiallyCharged = false;
  } else {

    chargeCounter += _elapsed;
    if (chargeCounter >= CHARGE_COUNTER_MAX) {
      if (isCharged == false) {
        // We're switching states
        AUDIO.Play(AudioType::BUSTER_CHARGED);
        animation.SetAnimation("CHARGED");
        animation << Animate::Mode::Loop;
      }

      isCharged = true;      
    } else if (chargeCounter >= CHARGE_COUNTER_MIN) {
      if (isPartiallyCharged == false) {
        // Switching states
        AUDIO.Play(AudioType::BUSTER_CHARGING);
        animation.SetAnimation("CHARGING");
        animation << Animate::Mode::Loop;
      }

      isPartiallyCharged = true;
    }

    if (isPartiallyCharged) {
      chargeSprite.setScale(2.0f, 2.0f);
    }
  }

  animation.Update(_elapsed, &chargeSprite);
  chargeSprite.setPosition(entity->getPosition().x, entity->getPosition().y - 36.0f);
}

void ChargeComponent::SetCharging(bool _charging) {
  charging = _charging;
}

float ChargeComponent::GetChargeCounter() const {
  return chargeCounter;
}

const bool ChargeComponent::IsFullyCharged() const
{
  return isCharged;
}

Sprite& ChargeComponent::GetSprite() {
  return chargeSprite;
}