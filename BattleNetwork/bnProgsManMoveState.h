/*! \brief This state determines how progsman moves and his next action based on his position */

#include "bnTile.h"
#include "bnField.h"
class ProgsMan;

class ProgsManMoveState : public AIState<ProgsMan>
{
private:
  Direction nextDirection;
  bool isMoving;
public:

  /**
   * @brief Sets isMoving to false
   * @param p
   */
  ProgsManMoveState();
  
  /**
   * @brief Deconstructor. Does nothing
   * @param p
   */
  ~ProgsManMoveState();

<<<<<<< HEAD
  void OnEnter(ProgsMan& p);
  void OnUpdate(float _elapsed, ProgsMan& p);
=======
  /**
   * @brief does nothing
   * @param p progsman entity
   */
  void OnEnter(ProgsMan& p);
  
  /**
   * @brief Calculates the next action
   * 
   * If the player is on the row, goto ProgsManShootState
   * If there is an Obstacle in front, always punch it
   * Randomly choose to throw a bomb
   * 
   * Line up with the player's Y tile and move to it
   * 
   * If all else fails move to another tile aimlessly
   * 
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

