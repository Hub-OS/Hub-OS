#pragma once
#include "bnArtifact.h"
#include "bnField.h"

/**
 * @class ChargedBusterHit
 * @author mav
 * @date 05/05/19
 * @file bnChargedBusterHit.h
 * @brief Animated hit effect on the enemy and then removes itself 
 */
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

