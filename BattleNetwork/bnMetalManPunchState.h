#pragma once
#include "bnTile.h"
#include "bnField.h"
class MetalMan;

<<<<<<< HEAD
=======
/*! \brief Metalman punches a tile, cracking or breaking it, and damages all entities on tile */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
class MetalManPunchState : public AIState<MetalMan>
{
public:
  MetalManPunchState();
  ~MetalManPunchState();

<<<<<<< HEAD
  void OnEnter(MetalMan& m);
  void OnUpdate(float _elapsed, MetalMan& m);
  void OnLeave(MetalMan& m);
=======
  /**
   * @brief Sets animation to punch, adds counterframes, calls Attack() on frame 4
   * @param m metalman
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
   * @brief Cracks tile. If already cracked, breaks tile. Hits all entities on tile
   * @param m metalman
   */
>>>>>>> b486e21e11627262088deae73097eaa7af56791c
  void Attack(MetalMan& m);
};

