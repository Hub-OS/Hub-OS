#pragma once
#include "bnAIState.h"
#include "bnCanodumb.h"

/**
 * @class CanodumbAttackState
 * @author mav
 * @date 05/05/19
 * @file bnCanodumbAttackState.h
 * @brief Spawns a cannon and smoke effect
 */
class CanodumbAttackState : public AIState<Canodumb>
{
public:
  CanodumbAttackState();
  ~CanodumbAttackState();

  void OnEnter(Canodumb& can);
  void OnUpdate(float _elapsed, Canodumb& can);
  void OnLeave(Canodumb& can);
};

