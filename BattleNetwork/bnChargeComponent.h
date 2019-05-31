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

/**
 * @class ChargeComponent
 * @author mav
 * @date 05/05/19
 * @brief Draws on top of attached entity
 */
class ChargeComponent : public SpriteSceneNode {
public:
  ChargeComponent(Entity* _entity);
  ~ChargeComponent();

  void Update(float _elapsed);
  
  /**
   * @brief If true, the component begins to charge. Otherwise, cancels charge
   * @param _charging
   */
  void SetCharging(bool _charging);
  float GetChargeCounter() const;
  
  /**
   * @brief Check full charge time
   * @return true if the charge component is at peak charge time
   */
  const bool IsFullyCharged() const;

private:
  Entity * entity;
  bool charging;
  bool isCharged;
  bool isPartiallyCharged;
  float chargeCounter;
  Animation animation;
};