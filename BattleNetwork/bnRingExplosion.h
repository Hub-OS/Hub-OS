#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class RingExplosion : public Artifact {
private:
  Animation animation;
  sf::Sprite poof;
public:
  /**
   * \brief sets the animation
   */
  RingExplosion(Field* field);

  /**
   * @brief deconstructor
   */
  ~RingExplosion();

  /**
   * @brief plays the animation and deletes when finished
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed);

  /**
   * @brief removes the ring explosion from play
   */
  void OnDelete();

  /**
   * @brief ring explosion effect doesn't move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction);
};
