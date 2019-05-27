#pragma once
#include "bnTile.h"
#include "bnField.h"
class MetalMan;

<<<<<<< HEAD
=======
/*! \brief Metalman throws a MetalBlade */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class MetalManThrowState : public AIState<MetalMan>
{
public:
  MetalManThrowState();
  ~MetalManThrowState();

<<<<<<< HEAD
  void OnEnter(MetalMan& m);
  void OnUpdate(float _elapsed, MetalMan& m);
  void OnLeave(MetalMan& m);
=======
  /**
   * @brief Animation is set to throw. Counter frames are set. Blade is spawned with Attack() on frame 3
   * @param m
   */
  void OnEnter(MetalMan& m);
  
  /**
   * @brief does nothing
   * @param _elapsed
   * @param m
   */
  void OnUpdate(float _elapsed, MetalMan& m);
  
  /**
   * @brief does nothing
   * @param m
   */
  void OnLeave(MetalMan& m);
  
  /**
   * @brief Spawns a MetalBlade and sets metal man as the aggressor
   * @param m metalman
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Attack(MetalMan& m);
};