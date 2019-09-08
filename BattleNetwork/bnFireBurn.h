#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class FireBurn : public Spell {
public:
  enum class Type : int {
    _1 = 1,
    _2 = 2,
    _3 = 3
  };

protected:
  Animation animation; /*!< the animation of the shot */
  int damage; /*!< How much damage to deal */
public:
  FireBurn(Field* _field, Team _team, Type type, int damage);

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
