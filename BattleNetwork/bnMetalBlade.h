#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

/*! \brief metal blade attack U-turns at end of field */
class MetalBlade : public Spell {
protected:
  AnimationComponent* animation; /*!< Blade spinnig animation */
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
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief Deals hitbox damage
   * @param _entity
   */
  virtual void Attack(Character* _entity);
};