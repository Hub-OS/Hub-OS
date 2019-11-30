/*! \brief Draws a normal sword swipe animation across a tile */
#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class SwordEffect : public Artifact {
private:
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
  virtual void OnUpdate(float _elapsed);
  
  /**
   * @brief Effect doesnt move across tiles
   * @param _direction dismissed
   * @return false
   */
  virtual bool Move(Direction _direction) { return false; }
};
