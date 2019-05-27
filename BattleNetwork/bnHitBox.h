#pragma once
#include "bnSpell.h"

<<<<<<< HEAD
class HitBox : public Spell {
public:
  HitBox(Field* _field, Team _team, int damage);
  virtual ~HitBox(void);

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction);
  virtual void Attack(Character* _entity);
private:
  int damage;
  float cooldown;
=======
/*! \brief Hitbox is an invisible spell used for more complex attack behaviors or used as body hitboxes
 * 
 * Because some attacks have lagging behavior, dropping a hitbox is more effective design
 * than allowing on the attack itself to deal damage. Some bosses can be moved into as well
 * and need to account for hitting the player. Overall hitbox is a spell that can be used as a tool
 * to design more complex attacks that do not behave in a simple way 
 */
class HitBox : public Spell {
public:

  /**
   * @brief disables tile highlighting by default
   */
  HitBox(Field* _field, Team _team, int damage);
  
  /**
   * @brief deconstructor
   */
  virtual ~HitBox();

  /**
   * @brief Attacks tile and deletes itself
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Does not move
   * @param _direction
   * @return false
   */
  virtual bool Move(Direction _direction);
  
  /**
   * @brief Deals hitbox property damage and then deletes itself
   * @param _entity entity to attack
   */
  virtual void Attack(Character* _entity);
private:
  int damage; /*!< how many units of damage to deal */
  bool hit; /*!< Flag if hit last frame */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
};
