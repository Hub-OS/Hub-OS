<<<<<<< HEAD
=======
/*! \brief Poof effect that plays when some chips fail */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class ParticlePoof : public Artifact {
private:
  Animation animation;
  sf::Sprite poof;
public:
<<<<<<< HEAD
  ParticlePoof(Field* field);
  ~ParticlePoof();

  virtual void Update(float _elapsed);
=======
  /**
   * \brief sets the animation 
   */
  ParticlePoof(Field* field);
  
  /**
   * @brief deconstructor
   */
  ~ParticlePoof();

  /**
   * @brief plays the animation and deletes when finished 
   * @param _elapsed in seconds
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief particle poof effect doesn't move
   * @param _direction ignored
   * @return false
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  virtual bool Move(Direction _direction) { return false; }
};