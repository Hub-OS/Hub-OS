#pragma once
#include "bnDefenseRule.h"
class BattleSceneBase;

class CounterCombatRule final : public DefenseRule {
  BattleSceneBase* battleScene{ nullptr };
public:
  CounterCombatRule(BattleSceneBase* battleScene);
  ~CounterCombatRule();
  void CanBlock(DefenseFrameStateJudge& judge, std::shared_ptr<Entity> attacker, std::shared_ptr<Entity> owner) override final;
};