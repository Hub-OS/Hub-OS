#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class MobMoveEffect : public Artifact {
private:
  Animation animation;
  sf::Sprite move;
  sf::Vector2f offset{};
public:
  /**
   * \brief sets the animation
   */
  MobMoveEffect();

  /**
   * @brief deconstructor
   */
  ~MobMoveEffect();

  /**
   * @brief plays the animation and deletes when finished
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) final override;

  /**
  * @brief Removes entity from play
  */
  void OnDelete() final override;

  /**
   * @brief mob move effect doesn't move
   * @param _direction ignored
   * @return false
   */
  bool Move(Direction _direction) final override;

  void SetOffset(const sf::Vector2f& offset); 
};