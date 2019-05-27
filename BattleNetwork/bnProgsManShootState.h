<<<<<<< HEAD
=======
/*! \brief Spawns a bullet and goes to ProgsManIdleState */

>>>>>>> b486e21e11627262088deae73097eaa7af56791c
#pragma once
#include "bnTile.h"
#include "bnField.h"
class ProgsMan;

class ProgsManShootState : public AIState<ProgsMan>
{
public:
<<<<<<< HEAD
  ProgsManShootState();
  ~ProgsManShootState();

  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
=======
  /**
   * @brief Constructor. Does nothing else.
   */
  ProgsManShootState();
  
  /**
   * @brief Deconstructor. Does nothing else.
   */
  ~ProgsManShootState();

  /**
   * @brief Set animation and spawn buster on frame 3
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
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void OnLeave(ProgsMan& p);
};

