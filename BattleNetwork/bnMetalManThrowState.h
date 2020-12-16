#pragma once
#include "bnAIState.h"

class MetalMan;

/*! \brief Metalman throws a MetalBlade */
class MetalManThrowState : public AIState<MetalMan>
{
public:
  MetalManThrowState();
  ~MetalManThrowState();

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
  void OnUpdate(double _elapsed, MetalMan& m);
  
  /**
   * @brief does nothing
   * @param m
   */
  void OnLeave(MetalMan& m);
  
  /**
   * @brief Spawns a MetalBlade and sets metal man as the aggressor
   * @param m metalman
   */
  void Attack(MetalMan& m);
};