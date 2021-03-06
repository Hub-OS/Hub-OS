/*! \brief Spells are attacks that can do damage 
 * 
 * Spells obey entity per-tile movement 
 * For cards that attack multiple tiles, spawn mutliple Spells
 * By default, all attacks have FloatShoe enabled because they can go over empty tiles.
 * Spells have an animationComponent to load animations easier.
 */

#pragma once
#include "bnEntity.h"
#include "bnTile.h"
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

using sf::Texture;

class Spell : public virtual Entity {
public:

  /**
   * @brief Sets the layer to 1 (underneath characters, layer = 0) and enables FloatShoe
   */
  Spell(Team team);
  virtual ~Spell();

  /**
   * @brief Queried by Tile to highlight or not
   * @return Highlight mode
   */
  const Battle::Tile::Highlight GetTileHighlightMode() const;

  /**
   * @brief Implement OnUpdate required
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(double _elapsed) = 0;

  void Update(double _elapsed);

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
  * @brief Describes what happens when this attack collides with a character, regardless of defenses
  *
  * If this function is called it does not gaurantee the attack will do damage to the character
  * Use this for visual effects like bubble pop
  */
  virtual void OnCollision() { };

  /**
   * @brief Uses visitor double-dispatch pattern to add to the spell bucket 
   * @param tile
   */
  virtual void AdoptTile(Battle::Tile* tile);

  /**
   * @brief Toggle whether or not to highlight a tile
   * @param mode
   * 
   * FLASH - flicker every other frame
   * SOLID - Stay yellow
   * NONE  - this is the default. No effects are applied.
   */
  void HighlightTile(Battle::Tile::Highlight mode);

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

  void SetHeight(double height);

  void QueueAction(const ActionEvent& action) override;

  void EndCurrentAction() override;

protected:
  Battle::Tile::Highlight mode; /*!< Highlight occupying tile */
  Hit::Properties hitboxProperties; /*!< Hitbox properties used when an entity is hit by this attack */
  double heightOffset; /*!< When drawing, how high up this spell should be. Used for card attacks where busters must align.*/
};
