
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
  SharedHitbox(Spell* owner, float duration = 0.0f);
   ~SharedHitbox();

  /**
   * @brief Removes itself if time is up or the original source is deleted
   * @param _elapsed in seconds
   */
   void OnUpdate(float _elapsed) override;
  
  /**
   * @brief Does not move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;
  
  /**
   * @brief owner->Attack(_entity) is called 
   * @param _entity the character to attack that is not the same as the owner
   */
  void Attack(Character* _entity) override;
  
  void OnDelete() override;

  const float GetHeight() const override;
  
private:
  float cooldown; /*< When cooldown reaches zero, this hitbox removes */
  bool keepAlive; /*!< If duration is not set, the hitbox stays alive for long as the owner stays alive*/
  Spell* owner; /*!< When this hitbox is hit, the owner is hit */
};
