#pragma once
#include "bnArtifact.h"
#include "bnField.h"

<<<<<<< HEAD
=======
/**
 * @class ChargedBusterHit
 * @author mav
 * @date 05/05/19
 * @brief Animated hit effect on the enemy and then removes itself 
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class ChargedBusterHit : public Artifact
{
private:
  AnimationComponent animationComponent;

public:
  ChargedBusterHit(Field* _field, Character* hit);
  ~ChargedBusterHit();

  virtual void Update(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }

};

