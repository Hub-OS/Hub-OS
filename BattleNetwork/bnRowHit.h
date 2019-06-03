
/*! \brief A bright pink shot effect that shoots across the map dealing damage */

#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class RowHit : public Spell {
protected:
  Animation animation; /*!< the animation of the shot */
  int damage; /*!< How much damage to deal */
public:
  /**
   * @brief Sets the animation with 2 callbacks: spawn another RowHit and delete on animation end
   */
  RowHit(Field* _field, Team _team,int damage);
  
  /**
   * @brief Deconstructor
   */
  virtual ~RowHit();

  /**
   * @brief Attack the tile and animate
   * @param _elapsed
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Does not move
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction);
  
  /**
   * @brief Deals damage with default hit props
   * @param _entity
   */
  virtual void Attack(Character* _entity);
};
