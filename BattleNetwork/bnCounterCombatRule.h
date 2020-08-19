#pragma once
#include "bnDefenseRule.h"
class BattleScene;

class CounterCombatRule final : public DefenseRule {
  BattleScene* battleScene{ nullptr };
public:
  CounterCombatRule(BattleScene* battleScene);
  ~CounterCombatRule();
  void CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner) override final;
};