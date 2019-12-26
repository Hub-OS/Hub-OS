
/*! \brief Impact effect that plays when some attacks make contact with entities */

#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class ParticleImpact : public Artifact {
private:
  Animation animation;
  sf::Sprite fx;
  sf::Vector2f randOffset;
public:
  enum Type {
    GREEN,
    BLUE,
    YELLOW,
    FIRE,
    THIN
  };

  /**
   * \brief sets the animation
   */
  ParticleImpact(Type type);

  /**
   * @brief deconstructor
   */
  ~ParticleImpact();

  /**
   * @brief plays the animation and deletes when finished
   * @param _elapsed in seconds
   */
  void OnUpdate(float _elapsed);

  /**
   * @brief particle fx effect doesn't move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) { return false; }

  void OnSpawn(Battle::Tile& tile);
}; 
