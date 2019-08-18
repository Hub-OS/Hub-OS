#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"

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
class Cube : public Obstacle {
public:
  Cube(Field* _field, Team _team);
  virtual ~Cube();

  /**
   * @brief Keep sliding if moving in previous frame
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);

  virtual const bool OnHit(const Hit::Properties props);

  virtual void OnDelete();
  
  virtual const float GetHitHeight() const { return 0; }

  virtual void SetAnimation(std::string animation);
  
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

  static int currCubeIndex; 
  static int cubesRemovedCount; 
  static const int numOfAllowedCubesOnField;
  int cubeIndex;
  bool hit;
  double timer;
  Direction previousDirection;

  DefenseRule* virusBody;
};