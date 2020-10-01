#pragma once
#include "bnDefenseRule.h"
class BattleScene;

class CounterCombatRule final : public DefenseRule {
  BattleSceneBase* battleScene{ nullptr };
public:
  CounterCombatRule(BattleSceneBase* battleScene);
  ~CounterCombatRule();
  void CanBlock(DefenseFrameStateJudge& judge, Spell& in, Character& owner) override final;
};