
/*! \brief Poof effect that plays when some chips fail */

#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class ParticlePoof : public Artifact {
private:
  Animation animation;
  sf::Sprite poof;
public:
  /**
   * \brief sets the animation 
   */
  ParticlePoof();
  
  /**
   * @brief deconstructor
   */
  ~ParticlePoof();

  /**
   * @brief plays the animation and deletes when finished 
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief particle poof effect doesn't move
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction) { return false; }
};