#pragma once

#include "bnAIState.h"

class Falzar;

class FalzarMoveState : public AIState<Falzar> {
  float cooldown{ 2.f }; /*!< How long to wait before moving */
  int moveCount{ 0 }, maxMoveCount{ 1 };
public:

  /**
   * @brief Sets default cooldown
   */
  FalzarMoveState(int moveCount);

  /**
   * @brief deconstructor
   */
  ~FalzarMoveState();
  void OnEnter(Falzar&);
  void OnUpdate(double _elapsed, Falzar&);
  void OnLeave(Falzar&);
  void OnMoveComplete(Falzar&);
  void OnFinishIdle(Falzar&);
};