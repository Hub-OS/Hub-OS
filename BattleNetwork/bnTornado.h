#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class Tornado : public Spell {
protected:
  Animation animation; /*!< the animation of the shot */
  int damage; /*!< How much damage to deal */
public:
  Tornado(Field* _field, Team _team, int damage);

  /**
   * @brief Deconstructor
   */
  ~Tornado();

  /**
   * @brief Attack the tile and animate
   * @param _elapsed
   */
  void OnUpdate(float _elapsed) override;

  /**
   * @brief Does not move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) override;

  /**
   * @brief Deals damage with default hit props
   * @param _entity
   */
  void Attack(Character* _entity) override;

  /**
  * @brief Does nothing
  */
  void OnDelete() override;
};
