/*! \brief Progsman sets his animation to throw and spawns a bomb */

#pragma once
#include "bnAIState.h"

class ProgsMan;

class ProgsManThrowState : public AIState<ProgsMan>
{
private:
    Battle::Tile* lastTargetPos;

public:
  ProgsManThrowState();
  ~ProgsManThrowState();

  /**
   * @brief Set the animation to throw, spawn bomb on frame 4 
   * @param p the progsman entity 
   */
  void OnEnter(ProgsMan& p);
  
  /**
   * @brief Does nothing
   * @param _elapsed
   * @param p the progsman entity
   */
  void OnUpdate(double _elapsed, ProgsMan& p);
  
  /**
   * @brief Does nothing
   * @param p the progsman entity
   */
  void OnLeave(ProgsMan& p);
};

