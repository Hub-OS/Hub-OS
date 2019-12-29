#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

class TwinFang : public Spell {
public:
  enum class Type : int {
    ABOVE,
    BELOW,
    ABOVE_DUD,
    BELOW_DUD
  };

protected:
  AnimationComponent* animation;
  bool spreadOut, onEdgeOfMap;
  float spreadOffset;
  float flickeroutTimer;
  Type type;

public:

  TwinFang(Field* _field, Team _team, Type _type, int damage);

  /**
   * @brief deconstructor
   */
  virtual ~TwinFang();

  /**
   * @brief Twin Fang through everything
   * @param tile
   * @return true
   */
  virtual bool CanMoveTo(Battle::Tile* tile);
  virtual void OnUpdate(float _elapsed);

  /**
   * @brief Deals hitbox damage
   * @param _entity
   */
  virtual void Attack(Character* _entity);
};
