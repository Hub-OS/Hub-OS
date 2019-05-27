#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

<<<<<<< HEAD
class MetalBlade : public Spell {
protected:
  Animation animation;
  double speed;
public:
  MetalBlade(Field* _field, Team _team, double speed = 1.0);
  virtual ~MetalBlade(void);
  virtual bool CanMoveTo(Battle::Tile* tile);
  virtual void Update(float _elapsed);
=======
/*! \brief metal blade attack U-turns at end of field */
class MetalBlade : public Spell {
protected:
  Animation animation; /*!< Blade spinnig animation */
  double speed; /*!< Faster spinning blades */
public:

  /**
   * @brief Sets animation and speed modifier
   * @param tile
   * 
   * Speed modifier changes sliding/gliding time
   */
  MetalBlade(Field* _field, Team _team, double speed = 1.0);
  
  /**
   * @brief deconstructor
   */
  virtual ~MetalBlade();
  
  /**
   * @brief Blade cuts through everything
   * @param tile
   * @return true
   */
  virtual bool CanMoveTo(Battle::Tile* tile);
  
  /**
   * @brief Moves left/right depending on team. Makes U-Turn at end of field.
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Deals hitbox damage
   * @param _entity
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual void Attack(Character* _entity);
};