#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

class TwinFang : public Spell {
public:
  enum class Type : int {
    ABOVE,
    BELOW
  };

protected:
  AnimationComponent* animation;
  bool spreadOut, onEdgeOfMap;
  float spreadOffset;
  double flickeroutTimer;
  Type type;

public:

  TwinFang(Team _team,Type _type, int damage);

  /**
   * @brief deconstructor
   */
  ~TwinFang();

  /**
   * @brief Twin Fang through everything
   * @param tile
   * @return true
   */
  bool CanMoveTo(Battle::Tile* tile) override;

  /**
  * @brief Spreads outwards on a target tile and then moves across the field
  * @param _elapsed float of elapsed time in seconds
  */
  void OnUpdate(double _elapsed) override;

  /**
   * @brief Deals hitbox damage
   * @param _entity
   */
  void Attack(Character* _entity) override;

  /**
  * Does nothing
  */
  void OnDelete() override;
};
