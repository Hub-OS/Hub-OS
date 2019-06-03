
/*! \brief External hit boxes that delegate hits to their owners
 * 
 * Some spells and characters drop lagging hit boxes @see Wave 
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
#include "bnObstacle.h"
#include "bnComponent.h"

// Hit boxes can attack and be attacked: obstacle traits
class SharedHitBox : public Obstacle {
public:
	
  SharedHitBox(Spell* owner, float duration);
  virtual ~SharedHitBox();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
  virtual const bool Hit(Hit::Properties props);
  
private:
  float cooldown;
  Spell* owner;
class SharedHitBox : public Obstacle {
public:
	
  /**
   * @brief Disables tile highlighting and enables tile sharing
   * @param owner the original spell source
   * @param duration how long the hitbox should linger in seconds
   */
  SharedHitBox(Spell* owner, float duration);
  virtual ~SharedHitBox();

  /**
   * @brief Removes itself if time is up or the original source is deleted
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Does not move
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction);
  
  /**
   * @brief owner->Attack(_entity) is called 
   * @param _entity the character to attack that is not the same as the owner
   */
  virtual void Attack(Character* _entity);
  
  /**
   * @brief If the owner is also an obstacle, calls owner->Attack(_entity)
   * @param props the hit properties of the aggressor
   * @return true if the owner was hit, false otherwise
   */
  virtual const bool Hit(Hit::Properties props);
  virtual const bool OnHit(Hit::Properties props) { return true; }
  virtual void OnDelete() { ; }

  virtual const float GetHitHeight() const { return 0;  }
  
private:
  float cooldown; /*< When cooldown reaches zero, this hitbox removes */
  Spell* owner; /*!< When this hitbox is hit, the owner is hit */
};
