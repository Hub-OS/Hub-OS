#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class FireBurn : public Spell {
protected:
  Animation animation; /*!< the animation of the shot */
  int damage; /*!< How much damage to deal */
public:
  FireBurn(Field* _field, Team _team, int damage);

  /**
   * @brief Deconstructor
   */
  virtual ~FireBurn();

  /**
   * @brief Attack the tile and animate
   * @param _elapsed
   */
  virtual void OnUpdate(float _elapsed);

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
