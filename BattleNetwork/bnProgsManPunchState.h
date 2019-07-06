/*! \brief Progsman punches and pushes anything in front of him */

#pragma once
#include "bnAIState.h"

class ProgsMan;

class ProgsManPunchState : public AIState<ProgsMan>
{
public:
  ProgsManPunchState();
  ~ProgsManPunchState();

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
  void Attack(ProgsMan& p);
};

