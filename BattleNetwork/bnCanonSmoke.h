#pragma once
#include "bnArtifact.h"
#include "bnField.h"

/**
 * @class CanonSmoke
 * @author mav
 * @date 05/05/19
 * @brief Animates smoke and deletes after
 */
class CanonSmoke : public Artifact
{
private:
  AnimationComponent* animationComponent;

public:
  CanonSmoke(Field* _field);
  ~CanonSmoke();

  /**
   * @brief animates smoke
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Does not move
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction) { return false; }
};

