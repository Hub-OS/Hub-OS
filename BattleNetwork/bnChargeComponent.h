#pragma once
#include <SFML/Graphics.hpp>
#include "bnAnimation.h"
#include "bnLayered.h"

using sf::CircleShape;
using sf::Sprite;
using sf::Drawable;
using sf::Texture;
using sf::IntRect;
class Entity;

#define CHARGE_COUNTER_MIN .40f
#define CHARGE_COUNTER_MAX 2.4f

/*!
 * TODO: use component system
*/
class ChargeComponent : public LayeredDrawable {
public:
  ChargeComponent(Entity* _entity);
  ~ChargeComponent();

  void Update(float _elapsed);
  void SetCharging(bool _charging);
  float GetChargeCounter() const;
  const bool IsFullyCharged() const;

private:
  Entity * entity;
  bool charging;
  bool isCharged;
  bool isPartiallyCharged;
  float chargeCounter;
  Animation animation;
};