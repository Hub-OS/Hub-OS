#pragma once
#include "bnAnimationComponent.h"
#include "bnArtifact.h"
#include "bnField.h"

class CanodumbIdleState;

/**
 * @class CanodumbCursor
 * @author mav
 * @date 05/05/19
 * @brief Spawned by ConodumbAttackState. Collides with player and makes cannon attack.
 * @warning Legacy code. Some code should be updated and utlize new behavior methods.
 */
class CanodumbCursor : public Artifact
{
private:
  CanodumbIdleState* parentState; /*!< The context of the Canodumb who spawned it */
  Entity* target; /*!< The enemy to track */
  float elapsedTime;
  float movecooldown; /*!< Time remaining between movement */
  float maxcooldown; /*!< Total time between movement */
  Direction direction; /*!< Direction to move */

  // Frame select through animation system
  AnimationComponent* animationComponent;
public:
  CanodumbCursor(Field* _field, Team _team, CanodumbIdleState* _parentState);
  ~CanodumbCursor();
  
  /**
   * @brief If tile is same as target, tells cannon to attack. Otherwise tries to move
   * @param _elapsed
   */
  void OnUpdate(float _elapsed);
  
  /**
   * @brief Legacy code. The cursor should be using this...
   * @param _direction
   * @return false
   */
  bool Move(Direction _direction) { return false; }
};
