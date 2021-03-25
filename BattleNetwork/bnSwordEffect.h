/*! \brief Draws a normal sword swipe animation across a tile */
#pragma once
#include "bnArtifact.h"
#include "bnComponent.h"
#include "bnField.h"

class SwordEffect : public Artifact {
public:
  /**
   * @brief loads the animation and adds a callback to delete when finished
   */
  SwordEffect();
  ~SwordEffect();

  /**
   * @brief Update the effect animation
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;

  /**
   * @brief Removes sword effect from play
   */
  void OnDelete() override;

  // On animation end it deletes itself
  void SetAnimation(const std::string& animation);
};
