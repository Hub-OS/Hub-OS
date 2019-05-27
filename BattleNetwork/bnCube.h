#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"

using sf::Texture;

<<<<<<< HEAD
/* 
    Cube has a timer of 100 seconds until it self destructs
    When it dies, it spawns debris
    When it is pushed, it slides until it cannot (always sliding)
    Floatshoe is disabled to crack tiles
    Cube has 200 HP 
*/

class Cube : public Obstacle {
public:
  Cube(Field* _field, Team _team);
  virtual ~Cube(void);

  virtual void Update(float _elapsed);
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);
  virtual void OnDelete();
  virtual void SetAnimation(std::string animation);
  virtual bool CanMoveTo(Battle::Tile * next);
  virtual void Attack(Character* e);
  double timer;
=======
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
  virtual void Update(float _elapsed);
  
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);
  
  virtual const bool OnHit(Hit::Properties props) { return true; }

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
>>>>>>> b486e21e11627262088deae73097eaa7af56791c

protected:
  Texture* texture;
  AnimationComponent animation;
  sf::Shader* whiteout;
<<<<<<< HEAD
  static int currCubeIndex;
  static int cubesRemovedCount;
  static const int numOfAllowedCubesOnField;
  int cubeIndex;
  bool hit;
=======
  static int currCubeIndex; 
  static int cubesRemovedCount; 
  static const int numOfAllowedCubesOnField;
  int cubeIndex;
  bool hit;
  double timer;
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
};