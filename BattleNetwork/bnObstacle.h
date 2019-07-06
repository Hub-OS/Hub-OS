
/*! \brief Obstacles are characters in the sense they can be spawned, hit, and have health
 *   but they are also used to damage entities occupying the same tile.
 * 
 *  Obstacles have health like cubes which can be destroyed
 *  Obstacles respond to being hit; some specific attacks only affect them
 *  Obstacles can also deal damage to other characters and obstacles
 *  For these reasons, obstacles are treated as 
 *  a third category: union of Character traits and Spell traits
 */
#pragma once
#include "bnCharacter.h"
#include "bnSpell.h"
#include "bnAnimationComponent.h"

using sf::Texture;
/*
    Obstacles are characters in the sense they can be spawned, but they generally deal damage
    to entities occupying the same tile.
*/

class Obstacle : public Character, public  Spell {
public:
  Obstacle(Field* _field, Team _team);
  virtual ~Obstacle();

  virtual void Update(float _elapsed) final override {
      Spell::Update(_elapsed);
      Character::Update(_elapsed);
  }

  /**
   * @brief Uses the Character::CanMoveTo() default function to follow typical character movement rules
   * @param next
   * @return 
   */
  virtual bool CanMoveTo(Battle::Tile * next) override;

  /**
   * @brief Uses the Spell::AdoptTile() function to be put into the Tile's spell bucket
   * @param tile
   */
  virtual void AdoptTile(Battle::Tile* tile) final override;
};