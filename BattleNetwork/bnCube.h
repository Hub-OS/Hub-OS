#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"

using sf::Texture;

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

protected:
  Texture* texture;
  AnimationComponent animation;
  sf::Shader* whiteout;
  static int currCubeIndex;
  static int cubesRemovedCount;
  static const int numOfAllowedCubesOnField;
  int cubeIndex;
  bool hit;
};