#pragma once
#include "bnSpell.h"
#include "bnAnimation.h"

class SuperVulcan : public Spell {
public:

protected:
  Animation animation; /*!< the animation of the shot */
public:
  SuperVulcan(Team _team, int damage);

  /**
   * @brief Deconstructor
   */
  ~SuperVulcan();

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
  * @brief does nothing
  */
  void OnDelete() override;
};
