#include "bnMobBattleScene.h"

#include "States/bnRewardBattleState.h"
#include "States/bnTimeFreezeBattleState.h"
#include "States/bnBattleStartBattleState.h"
#include "States/bnBattleOverBattleState.h"
#include "States/bnFadeOutBattleState.h"
#include "States/bnCombatBattleState.h"
#include "States/bnCharacterTransformBattleState.h"
#include "States/bnMobIntroBattleState.h"
#include "States/bnCardSelectBattleState.h"
#include "States/bnCardComboBattleState.h"

MobBattleScene::MobBattleScene(const MobBattleProperties& props) 
: props(props), BattleSceneBase(props.base) {
    // First, we create all of our scene states
    auto intro = AddState<MobIntroBattleState>();
    auto cardSelect = AddState<CardSelectBattleState>();
    auto combat = AddState<CombatBattleState>();
    auto combo = AddState<CardComboBattleState>();
    auto forms = AddState<CharacterTransformBattleState>();
    auto battlestart = AddState<BattleStartBattleState>();
    auto battleover = AddState<BattleOverBattleState>();
    auto timeFreeze = AddState<TimeFreezeBattleState>();
    auto reward = AddState<RewardBattleState>();
    auto fadeout = AddState<FadeOutBattleState>(this, FadeOut::black); // this state requires arguments

    // the macro below is the same as this formal C++ notation...
    /*intro.ChangeOnEvent(cardSelect,     &IntroMobBattleState::IsOver);
    cardSelect.ChangeOnEvent(battlestart, &CardSelectBattleState::OKIsPressed);
    battlestart.ChangeOnEvent(combat,     &BattleStartBattleState::IsFinished);
    battleover.ChangeOnEvent(reward,      &BattleOverBattleState::IsFinished);
    timeFreeze.ChangeOnEvent(combat,      &TimeFreezeattleState::IsOver);
    reward.ChangeOnEvent(fadeout,         &RewardBattleState::OKIsPressed);*/

    //        hange from        ,to          ,when this is true
    CHANGE_ON_EVENT(intro       ,cardSelect  ,IsOver);
    CHANGE_ON_EVENT(cardSelect  ,battlestart ,OKIsPressed);
    CHANGE_ON_EVENT(battlestart ,combat      ,IsFinished);
    CHANGE_ON_EVENT(battleover  ,reward      ,IsFinished);
    CHANGE_ON_EVENT(timeFreeze  ,combat      ,IsOver);
    CHANGE_ON_EVENT(reward      ,fadeout     ,OKIsPressed);

    // combat has multiple state interruptions based on events
    // so we chain them together instead of using the macro
    combat  .ChangeOnEvent(battleover, &CombatBattleState::PlayerWon)
            .ChangeOnEvent(battleover, &CombatBattleState::PlayerLost)
            .ChangeOnEvent(cardSelect, &CombatBattleState::PlayerRequestCardSelect)
            .ChangeOnEvent(timeFreeze, &CombatBattleState::HasTimeFreeze);

    // this kicks-off the state graph beginning with the intro state
    this->StartStateGraph(intro);
}

MobBattleScene::~MobBattleScene() {

}

void MobBattleScene::onExit()
{
}

void MobBattleScene::onEnter()
{
}

void MobBattleScene::onResume()
{
}

void MobBattleScene::onEnd()
{
}
