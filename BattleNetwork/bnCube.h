#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"
#include "bnCounterTrait.h"

using sf::Texture;

/**
 * @class Cube
 * @author mav
 * @date 05/05/19
 * @brief A cube that, when pushed, slides across hitting everything in its path
 * 
 *  Cube has a timer of 100 seconds until it self destructs
 *  When it dies, it spawns debris
 *  When it is pushed, it slides until it cannot (always sliding)
 *  Floatshoe is disabled to crack tiles
 *  Cube has 200 HP 
 */
class Cube : public Obstacle, public CounterTrait<Cube> {
public:
  Cube(Field* _field, Team _team);
  virtual ~Cube();

  /**
   * @brief Keep sliding if moving in previous frame
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed);

  const bool OnHit(const Hit::Properties props);

  void OnDelete();
  
  const float GetHeight() const;

  void SetAnimation(std::string animation);

  void OnSpawn(Battle::Tile& start);
  
  /**
   * @brief Can move over any tile if it is not broken or empty or contains another cube
   * @param next tile to move to
   * @return true if it can move to the tile, false otherwise
   */
  virtual bool CanMoveTo(Battle::Tile * next);
  
  /**
   * @brief Deals 200 units of damage to enemies occupying the tile and then breaks
   * @param e
   */
  virtual void Attack(Character* e);

protected:
  Texture* texture;
  AnimationComponent* animation;
  sf::Shader* whiteout;

  static const int numOfAllowedCubesOnField;

  bool hit;
  bool pushedByDrag; /*!< Whether or not to keep momentum going*/
  double timer;
  Direction previousDirection;

  DefenseRule* virusBody;
};