#pragma once
#include "bnTile.h"
#include "bnField.h"
#include "bnAIState.h"

class MetalMan;

/*! \brief Metalman punches a tile, cracking or breaking it, and damages all entities on tile */
class MetalManPunchState : public AIState<MetalMan>
{
public:
  MetalManPunchState();
  ~MetalManPunchState();

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
  void Attack(MetalMan& m);
};

