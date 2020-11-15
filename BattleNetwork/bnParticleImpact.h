
/*! \brief Impact effect that plays when some attacks make contact with entities */

#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

enum class Type : int;

class ParticleImpact : public Artifact {
public:
  enum class Type : int {
    green,
    blue,
    yellow,
    fire,
    thin,
    vulcan,
    volcano,
    wind
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
  void OnUpdate(float _elapsed) final override;

  void OnDelete() final override;

  /**
   * @brief particle fx effect doesn't move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) final override;

  void OnSpawn(Battle::Tile& tile) final override;


  void SetOffset(const sf::Vector2f& offset);

private:
  Animation animation;
  sf::Vector2f randOffset, offset;
  Type type;
}; 
