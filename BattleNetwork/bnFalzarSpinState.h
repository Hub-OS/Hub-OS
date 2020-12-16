#pragma once

#include "bnAIState.h"

class Falzar;

class FalzarSpinState : public AIState<Falzar> {
  float speed{ .08f }; // seconds
  int spinCount{ 0 }, maxSpinCount{ 4 };
public:

  /**
   * @brief Sets default cooldown
   */
  FalzarSpinState(int spinCount);

  /**
   * @brief deconstructor
   */
  ~FalzarSpinState();

  void OnEnter(Falzar&);
  void OnUpdate(double _elapsed, Falzar&);
  void OnLeave(Falzar&);
};