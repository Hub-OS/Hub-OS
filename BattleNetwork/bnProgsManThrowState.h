/*! \brief Progsman sets his animation to throw and spawns a bomb */

#pragma once
#include "bnTile.h"
#include "bnField.h"
class ProgsMan;

class ProgsManThrowState : public AIState<ProgsMan>
{
private:
    Battle::Tile* lastTargetPos;

public:
  ProgsManThrowState();
  ~ProgsManThrowState();

<<<<<<< HEAD
  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
=======
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
  void OnUpdate(float _elapsed, ProgsMan& p);
  
  /**
   * @brief Does nothing
   * @param p the progsman entity
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void OnLeave(ProgsMan& p);
};

