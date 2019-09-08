#pragma once
#include "bnArtifact.h"
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

class Field;

/**
 * @class ChargedBusterHit
 * @author mav
 * @date 05/05/19
 * @brief Animated hit effect on the enemy and then removes itself 
 */
class ChargedBusterHit : public Artifact
{
private:
  AnimationComponent* animationComponent;

public:
  ChargedBusterHit(Field* _field);
  ~ChargedBusterHit();

  virtual void OnUpdate(float _elapsed);
  virtual bool Move(Direction _direction) { return false; }

};

