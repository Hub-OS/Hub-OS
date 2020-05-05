#pragma once
#include "bnAIState.h"
#include "bnCanodumb.h"

/**
 * @class CanodumbAttackState
 * @author mav
 * @date 05/05/19
 * @brief Spawns a cannon and smoke effect
 */
class CanodumbAttackState final : public AIState<Canodumb>
{
public:
  CanodumbAttackState();
  ~CanodumbAttackState();

  void OnEnter(Canodumb& can) override;
  void OnUpdate(float _elapsed, Canodumb& can) override;
  void OnLeave(Canodumb& can) override;
};

