#pragma once

#include "bnArtifact.h"
#include "bnField.h"

#include "bnCanodumb.h"
#include "bnCanodumbAttackState.h"

/**
 * @class CanodumbCursor
 * @author mav
 * @date 05/05/19
 * @file bnCanodumbCursor.h
 * @brief Spawned by ConodumbAttackState. Collides with player and makes cannon attack.
 * @warning Legacy code. Some code should be updated and utlize new behavior methods.
 */
class CanodumbCursor : public Artifact
{
private:
  Canodumb* parent; /*!< The Canodumb who spawned it */
  Entity* target; /*!< The enemy to track */
  float movecooldown; /*!< Time remaining between movement */
  float maxcooldown; /*!< Total time between movement */
  Direction direction; /*!< Direction to move */

  // Frame select through animation system
  AnimationComponent animationComponent;
public:
  CanodumbCursor(Field* _field, Team _team, Canodumb* _parent);
  ~CanodumbCursor();
  
  /**
   * @brief If tile is same as target, tells cannon to attack. Otherwise tries to move
   * @param _elapsed
   */
  virtual void Update(float _elapsed);
  
  /**
   * @brief Legacy code. The cursor should be using this...
   * @param _direction
   * @return false
   */
  virtual bool Move(Direction _direction) { return false; }
};


