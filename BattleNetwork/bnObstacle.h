
/*! \brief Obstacles are characters in the sense they can be spawned, hit, and have health
 *   but they are also used to damage entities occupying the same tile.
 * 
 *  Obstacles have health like cubes which can be destroyed
 *  Obstacles respond to being hit; some specific attacks only affect them
 *  Obstacles can also deal damage to other characters and obstacles
 */
#pragma once
#include "bnCharacter.h"
#include "bnAnimationComponent.h"

using sf::Texture;
/*
    Obstacles are characters in the sense they can be spawned, but they generally deal damage
    to entities occupying the same tile.
*/

class Obstacle : public Character
{
public:
  Obstacle(Team _team);
};