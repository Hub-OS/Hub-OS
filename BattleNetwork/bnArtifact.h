#pragma once
#include "bnEntity.h"

/**
 * @brief Artifacts do not attack and they are not living. They are tile-rooted animations purely for visual effect.
 */
class Artifact : public Entity {
public:
  Artifact();

  virtual void OnUpdate(double _elapsed) = 0;
  virtual void OnDelete() = 0;
  void Update(double _elapsed) override final;
};
