/*! \brief Part of roll summon. Heart appears from the top of the screen slowly descending 
 * 
 * NOTE: The chip summon system is going under major refactoring and this
 * code will not be the same
 * 
 * NOTE: This should just be an artifact since it is used as effect
 */

#pragma once

#include "bnSpell.h"
#include "bnChipSummonHandler.h"

class RollHeart : public Spell {
public:
  RollHeart(ChipSummonHandler* _summons, int _heal);
<<<<<<< HEAD
  virtual ~RollHeart(void);
=======
  virtual ~RollHeart();
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

  /**
   * @brief Descend and then heal the player
   * @param _elapsed
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief does not move
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction);
<<<<<<< HEAD
  virtual void Attack(Character* _entity);

private:
  int heal;
  float height = 200;
  Character* caller;
  ChipSummonHandler* summons;
  bool doOnce;
=======
  
  /**
   * @brief Does nothing
   * @param _entity
   */
  virtual void Attack(Character* _entity);

private:
  int heal; /*!< How much to heal */
  float height; /*!< The start height of the heart */
  Character* caller; /*!< The character that used the chip */
  ChipSummonHandler* summons; /*!< The chip summon system */
  bool doOnce; /*!< Flag to restore health once */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
}; 
