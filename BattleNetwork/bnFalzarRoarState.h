#pragma once

#include "bnAIState.h"

class Falzar;

class FalzarRoarState : public AIState<Falzar> {
  int animateCount{ 16 };
  int maxAnimateCount{ 16 };
public:

  /**
   * @brief Sets default cooldown
   */
  FalzarRoarState();

  /**
   * @brief deconstructor
   */
  ~FalzarRoarState();


  void OnEnter(Falzar&);
  void OnUpdate(float _elapsed, Falzar&);
  void OnLeave(Falzar&);
};