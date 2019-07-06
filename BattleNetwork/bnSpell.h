/*! \brief Spells are attacks that can do damage 
 * 
 * Spells obey entity per-tile movement 
 * For chips that attack multiple tiles, spawn mutliple Spells
 * By default, all attacks have FloatShoe enabled because they can go over empty tiles.
 * Spells have an animationComponent to load animations easier.
 */

#pragma once
#include "bnEntity.h"
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

using sf::Texture;

class Spell : public virtual Entity {
public:

  /**
   * @brief Sets the layer to 1 (underneath characters, layer = 0) and enables FloatShoe
   */
  Spell(Field* field, Team team);
  virtual ~Spell();

  /**
   * @brief Queried by Tile to highlight or not
   * @return true if markTile is true, otherwise false
   */
  const bool IsTileHighlightEnabled() const;

  /**
   * @brief Implement OnUpdate required
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed) = 0;

  void Update(float _elapsed);

  /**
   * @brief Describes how the spell attacks characters
   * @param _entity
   * 
   * This is where you would check for specific character types
   * if there are special conditions. Call Hit() on the entity
   * to deal damage.
   */
  virtual void Attack(Character* _entity) = 0;
  
  /**
   * @brief Uses visitor double-dispatch pattern to add to the spell bucket 
   * @param tile
   */
  virtual void AdoptTile(Battle::Tile* tile);

  /**
   * @brief Toggle whether or not to highlight a tile
   * @param enable
   * 
   * Can be toggled on over a short period of time to make the 
   * yellow flashing effect seen in some attacks
   */
  void EnableTileHighlight(bool enable);

  /**
   * @brief When an entity is hit, these are the hit properties it will use
   * @param props
   */
  void SetHitboxProperties(Hit::Properties props);
  
  /**
   * @brief Get the hitbox properties of this spell
   * @return const Hit::Properties
   */
  const Hit::Properties GetHitboxProperties() const;

protected:
  bool markTile; /*!< Highlight occupying tile */
  Hit::Properties hitboxProperties; /*!< Hitbox properties used when an entity is hit by this attack */
};
