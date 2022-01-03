#pragma once
#include "bnHitProperties.h"

class Entity;
class HitPublisher;

/**
 * @class HitListener
 * @author mav
 * @date 11/11/20
 * @brief Respond to a counter hit
 *
 * Some attacks, enemies, or scene logic respond to normal hits from Characters
 */
class HitListener {
public:
  HitListener() = default;
  ~HitListener() = default;

  HitListener(const HitListener& rhs) = delete;
  HitListener(HitListener&& rhs) = delete;

  /**
   * @brief Describe what happens when recieving hit information
   * @param victim who was countered
   * @param hitbox properties that harmed the victim
   */
  virtual void OnHit(Entity& victim, const Hit::Properties& props) = 0;

  /**
   * @brief Subscribe to a potential publisher
   * @param publisher source that a hit event can emit from
   */
  void Subscribe(HitPublisher& publisher);
};