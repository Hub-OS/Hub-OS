#pragma once
#include "bnEntity.h"
#include "stx/memory.h"
using sf::Texture;

/**
 * @brief Artifacts do not attack and they are not living. They are tile-rooted animations purely for visual effect.
 */
class Artifact : public Entity, public stx::enable_shared_from_base<Artifact> {
public:
  Artifact();
  virtual ~Artifact();

  virtual void OnUpdate(double _elapsed) = 0;
  virtual void OnDelete() = 0;
  void Update(double _elapsed) override final;
  void AdoptTile(Battle::Tile* tile) override final;
};
