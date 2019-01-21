#pragma once
#include <SFML/Graphics.hpp>
#include "bnAnimation.h"

using sf::CircleShape;
using sf::Sprite;
using sf::Drawable;
using sf::Texture;
using sf::IntRect;
class Entity;

#define CHARGE_COUNTER_MIN .40f
#define CHARGE_COUNTER_MAX 2.4f

/*!
 * @brief For player only right now
*/
class ChargeComponent {
public:
  ChargeComponent(Entity* _entity);
  ~ChargeComponent();

  void Update(float _elapsed);
  void SetCharging(bool _charging);
  float GetChargeCounter() const;
  const bool IsFullyCharged() const;
  Sprite& GetSprite();

private:
  Entity * entity;
  bool charging;
  bool isCharged;
  bool isPartiallyCharged;
  float chargeCounter;
  Sprite chargeSprite;
  Animation animation;
};