#pragma once
#pragma once
#include "bnArtifact.h"
#include "bnField.h"

/**
 * @class GuardHit
 * @author mav
 * @date 01/05/19
 * @brief Green _dink_ effect when guard is active
 */
class GuardHit : public Artifact
{
private:
  AnimationComponent animationComponent; /*!< Animate the effect */
  float w; float h; /*!< Area to appear in */
  bool center; /*!< If true, dink will appear in center of area instead of random */

public:
  /**
   * @brief Load the animation and set position
   */
  GuardHit(Field* _field, Character* hit, bool center = false);
  ~GuardHit();

  /**
   * @brief When the animation ends, it deletes itself
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

