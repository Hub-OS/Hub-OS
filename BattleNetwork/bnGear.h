#pragma once
#include "bnObstacle.h"
#include "bnAnimationComponent.h"

using sf::Texture;

/**
 * @class Gear
 * @author mav
 * @date 04/05/19
 * @file bnGear.h
 * @brief Gear slides left and right slowly
 * 
 * Gear cannot collide with other obstacles
 * If it's coming up on one, it reverses direction
 * Gears are impervious to damage
 */
class Gear : public Obstacle {
public:
  Gear(Field * _field, Team _team, Direction startDir);
  virtual ~Gear();

  /**
   * @brief Moves in direction and only within its team area
   * @param _elapsed in seconds
   * 
   * if battle is not activive, does not move
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Returns true always
   * @param props ignored
   * @return true
   */
  virtual const bool Hit( Hit::Properties props = Hit::DefaultProperties);
  
  /**
   * @brief No special behavior occur when deleted
   */
  virtual void OnDelete();
  
  /**
   * @brief Checks if it can move to a tile. If another gear is present, reverse direction.
   * @param next the tile to check
   * @return true if gear can move, false otherwise. May change direction.
   */
  virtual bool CanMoveTo(Battle::Tile * next);
  
  /**
   * @brief Gears deal damage to entities without guard protection. Gears always destroy obstacles.
   * @param e
   */
  virtual void Attack(Character* e);

private:
  Direction startDir;
  Team tileStartTeam; // only move around on the origin team's area
  Texture* texture;
  AnimationComponent animation;
};
