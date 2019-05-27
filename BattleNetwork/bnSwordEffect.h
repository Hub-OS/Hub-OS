<<<<<<< HEAD
=======
/*! \brief Draws a normal sword swipe animation across a tile */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class SwordEffect : public Artifact {
private:
<<<<<<< HEAD
  Animation animation;
public:
  SwordEffect(Field* field);
  ~SwordEffect();

  virtual void Update(float _elapsed);
=======
  Animation animation; /*!< Animation of the effect */
public:
  /**
   * @brief loads the animation and adds a callback to delete when finished
   */
  SwordEffect(Field* field);
  ~SwordEffect();

  /**
   * @brief Update the effect animation
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Effect doesnt move across tiles
   * @param _direction dismissed
   * @return false
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual bool Move(Direction _direction) { return false; }
};
