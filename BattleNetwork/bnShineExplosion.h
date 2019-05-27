<<<<<<< HEAD
=======
/*! \brief Boss shine that loops and draws on top of exploding navis */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnArtifact.h"
#include "bnField.h"
#include "bnAnimationComponent.h"

class ShineExplosion : public Artifact
{
private:
  AnimationComponent animationComponent;

public:
<<<<<<< HEAD
  ShineExplosion(Field* _field, Team _team);
  ~ShineExplosion();

  virtual void Update(float _elapsed);
=======
  /**
   * @brief Loads the shine texture and animation. Sets the layer to 0*
   */
  ShineExplosion(Field* _field, Team _team);
  ~ShineExplosion();

  /**
   * @brief Loops animations
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief the shine effect does not move
   * @param _direction ignored
   * @return false
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual bool Move(Direction _direction) { return false; }
};
