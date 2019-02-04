#pragma once
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

using sf::Texture;

/* 
    Cube has a timer of 100 seconds until it self destructs
    When it dies, it spawns debris
    When it is pushed, it slides until it cannot (always sliding)
    Floatshoe is disabled to crack tiles
    Cube has 200 HP 
*/

class Cube : public Character {
public:
  Cube(Field* _field, Team _team);
  virtual ~Cube(void);

  virtual void Update(float _elapsed);
  virtual const bool Hit(int damage, Hit::Properties props = Entity::DefaultHitProperties);
  virtual void SetAnimation(std::string animation);
  virtual bool CanMoveTo(Battle::Tile * next);

  vector<Drawable*> GetMiscComponents() { return vector<Drawable*>(); }
  double timer;
protected:
  Texture* texture;
  AnimationComponent animation;
  sf::Shader* whiteout;
};