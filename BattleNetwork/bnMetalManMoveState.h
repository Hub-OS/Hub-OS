#pragma once
#include "bnTile.h"
#include "bnField.h"
class MetalMan;

<<<<<<< HEAD
=======
/*! \brief Moves metalman and decides his next state based on target 
 */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class MetalManMoveState : public AIState<MetalMan>
{
private:
  Direction nextDirection;
<<<<<<< HEAD
  bool isMoving;
  bool mustMove;
=======
  bool isMoving; /*!< True if metalman's move animation is playing */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
public:
  MetalManMoveState();
  ~MetalManMoveState();

<<<<<<< HEAD
  void OnEnter(MetalMan& m);
  void OnUpdate(float _elapsed, MetalMan& m);
=======
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
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void OnLeave(MetalMan& m);
};

