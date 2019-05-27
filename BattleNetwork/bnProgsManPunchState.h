/*! \brief Progsman punches and pushes anything in front of him */

#pragma once
#include "bnTile.h"
#include "bnField.h"
class ProgsMan;

class ProgsManPunchState : public AIState<ProgsMan>
{
public:
  ProgsManPunchState();
  ~ProgsManPunchState();

<<<<<<< HEAD
  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
  void OnLeave(ProgsMan& p);
=======
  /**
   * @brief Set animation. On end frame calls Attack()
   * @param p progsman entity
   */
  void OnEnter(ProgsMan& p);
  
  /**
   * @brief does nothing
   * @param _elapsed
   * @param p progsman entity
   */
  void OnUpdate(float _elapsed, ProgsMan& p);
  
  /**
   * @brief does nothing
   * @param p progsman entity
   */
  void OnLeave(ProgsMan& p);
  
  /**
   * @brief Spawns a hurtbox on the tile and pushes every character on the tile
   * @param p progsman entity
   * 
   * Sets the next state to ProgsmanIdleState
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Attack(ProgsMan& p);
};

