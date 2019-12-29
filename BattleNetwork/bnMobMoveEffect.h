#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class MobMoveEffect : public Artifact {
private:
  Animation animation;
  sf::Sprite move;
public:
  /**
   * \brief sets the animation
   */
  MobMoveEffect(Field* field);

  /**
   * @brief deconstructor
   */
  ~MobMoveEffect();

  /**
   * @brief plays the animation and deletes when finished
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);

  /**
   * @brief mob move effect doesn't move
   * @param _direction ignored
   * @return false
   */
  virtual bool Move(Direction _direction) { return false; }
};