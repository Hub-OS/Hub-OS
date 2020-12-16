#pragma once
#pragma once
#include "bnSpell.h"
#include "bnAnimationComponent.h"

/*! \brief YoYo moves 3 spaces forward, if able, hits 3 times, and then returns to their starting tile*/
class YoYo : public Spell {
protected:
  AnimationComponent* animation; /*!< YoYo spinning animation */
  double speed; /*!< Faster spinning */
  int tileCount, hitCount;
  bool reversed;
  Battle::Tile* startTile;
public:

  /**
   * @brief Sets animation and speed modifier
   * @param tile
   *
   * Speed modifier changes sliding/gliding time
   */
  YoYo(Field* _field, Team _team, int damage, double speed = 1.0);

  /**
   * @brief deconstructor
   */
  ~YoYo();

  /**
   * @brief YoYo cuts through everything
   * @param tile
   * @return true
   */
  bool CanMoveTo(Battle::Tile* tile);

  /**
   * @brief Counts tile movements to max 3 and then returns to owner tile
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed);

  /**
   * @brief Deals hitbox damage
   * @param _entity
   */
  void Attack(Character* _entity);

  /**
  * @brief If not on the return tile, spawn an explosion
  */
  void OnDelete();

  void OnSpawn(Battle::Tile& start);
};