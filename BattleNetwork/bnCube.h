#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"
#include "bnInstanceCountingTrait.h"

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
class Cube : public Obstacle, public InstanceCountingTrait<Cube> {
public:
  Cube(Field* _field, Team _team);
  ~Cube();

  /**
   * @brief Keep sliding if moving in previous frame
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed) override;

  void OnDelete() override;
  
  const float GetHeight() const;

  void SetAnimation(std::string animation);

  void OnSpawn(Battle::Tile& start) override;
  
  /**
   * @brief Can move over any tile if it is not broken or empty or contains another cube
   * @param next tile to move to
   * @return true if it can move to the tile, false otherwise
   */
  bool CanMoveTo(Battle::Tile * next) override;
  
  /**
   * @brief Deals 200 units of damage to enemies occupying the tile and then breaks
   * @param e
   */
  void Attack(Character* e) override;

  const bool DidSpawnCorrectly() const;

protected:
  static const int numOfAllowedCubesOnField;

  bool killLater;
  bool pushedByDrag; /*!< Whether or not to keep momentum going*/
  double timer;
  Direction previousDirection;
  DefenseRule* defense;
  AnimationComponent* animation;
};