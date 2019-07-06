#pragma once
#include "bnTile.h"
#include "bnField.h"
#include "bnMetalMan.h"

class MetalMan;

/*! \brief Moves metalman and decides his next state based on target 
 */
class MetalManMoveState : public AIState<MetalMan>
{
private:
  Direction nextDirection;
  bool isMoving; /*!< True if metalman's move animation is playing */
public:
  MetalManMoveState();
  ~MetalManMoveState();

  /**
   * @brief does nothing
   * @param m
   */
  void OnEnter(MetalMan& m);
  
  /**
   * @brief Finds a free tile in metalman's area to move to and may attack
   * @param _elapsed 
   * @param m
   * 
   * Attacks based on RNG and if player is on the back row, he will throw blades
   * Randomly Metalman will teleport to tile in front of player and punch the ground
   */
  void OnUpdate(float _elapsed, MetalMan& m);
  
  /**
   * @brief does nothing
   * @param m
   */
  void OnLeave(MetalMan& m);
};

