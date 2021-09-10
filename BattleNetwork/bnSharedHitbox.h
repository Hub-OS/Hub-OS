
/*! \brief External hit boxes that delegate hits to their owners
 * 
 * Some spells and characters drop lagging hit boxes 
 * and contact with these hitboxes need to behave the same as if
 * they hit the original source.
 * 
 * Shared hit boxes are classified as Obstacles because they can 
 * hit other characters and be hit themselves.
 * 
 * e.g. Boss hitbox can be hit and boss hitbox can deal damage to
 * player
 */

#pragma once
#include "bnSpell.h"
#include "bnComponent.h"

// If there needs to be a shared hit box for obstacles, it needs to be it's own class
class SharedHitbox : public Spell {
public:
	
  /**
   * @brief Disables tile highlighting and enables tile sharing
   * @param owner the original spell source
   * @param duration how long the hitbox should linger in seconds
   */
  SharedHitbox(std::weak_ptr<Spell> owner, float duration = 0.0f);
   ~SharedHitbox();

  /**
   * @brief Removes itself if time is up or the original source is deleted
   * @param _elapsed in seconds
   */
   void OnUpdate(double _elapsed) override;
  
  /**
   * @brief owner->Attack(_entity) is called 
   * @param _entity the entity to attack that is not the same as the owner
   */
  void Attack(std::shared_ptr<Entity> _entity) override;
  void OnDelete() override;
  void OnSpawn(Battle::Tile& start) override;
  const float GetHeight() const override;
  
private:
  double cooldown{}; /*< When cooldown reaches zero, this hitbox removes */
  bool keepAlive{}; /*!< If duration is not set, the hitbox stays alive for long as the owner stays alive*/
  std::weak_ptr<Spell> owner; /*!< When this hitbox is hit, the owner is hit */
};
