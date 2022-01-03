/*! \brief Spells are attacks that can do damage 
 * 
 * Spells obey entity per-tile movement 
 * For cards that attack multiple tiles, spawn mutliple Spells
 * By default, all attacks have FloatShoe enabled because they can go over empty tiles.
 * Spells have an animationComponent to load animations easier.
 */

#pragma once
#include "bnEntity.h"

using sf::Texture;

class Spell : public Entity {
public:
  /**
   * @brief Sets the layer to 1 (underneath characters, layer = 0) and enables FloatShoe
   */
  Spell(Team team);

  /**
   * @brief Implement OnUpdate required
   * @param _elapsed in seconds
   */
  void OnUpdate(double _elapsed) override;
};
