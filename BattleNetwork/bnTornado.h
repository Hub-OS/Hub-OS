#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class Tornado : public Spell {
protected:
  Animation animation; /*!< the animation of the shot */
  int damage; /*!< How much damage to deal */
  int count; /*!< How many times to hit */
public:
  Tornado(Team _team,int count, int damage);

  /**
   * @brief Deconstructor
   */
  ~Tornado();

  /**
   * @brief Attack the tile and animate
   * @param _elapsed
   */
  void OnUpdate(double _elapsed) override;

  /**
   * @brief Deals damage with default hit props
   * @param _entity
   */
  void Attack(std::shared_ptr<Character> _entity) override;

  /**
  * @brief Does nothing
  */
  void OnDelete() override;
};
