#pragma once

#include "bnAIState.h"

class Falzar;

class FalzarIdleState : public AIState<Falzar> {
  float cooldown{ 2.f }; /*!< How long to wait before moving */
public:

  /**
   * @brief Sets default cooldown 
   */
  FalzarIdleState();

  /**
   * @brief deconstructor
   */
  ~FalzarIdleState();

  /**
   * @brief Sets animation to IDLE
   * @param falzar
   *
   * If EX, cooldown is set to shorter time
   */
  void OnEnter(Falzar&);

  /**
   * @brief When cooldown hits zero, attacks or move
   * @param _elapsed in seconds
   * @param falzar
   */
  void OnUpdate(float _elapsed, Falzar&);

  /**
   * @brief Does nothing
   * @param falzar
   */
  void OnLeave(Falzar&);
};