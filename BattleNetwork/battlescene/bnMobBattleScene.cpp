#include "bnMobBattleScene.h"

MobBattleScene::MobBattleScene(const MobBattleProperties& props) 
: IBattleScene(*props.controller, props.player) {
    // First, we create all of our scene states
    auto intro = AddState<IntroMobState>();
    auto cardSelect = AddState<CardSelectState>();
    auto combat = AddState<CombatState>();
    auto combo = AddState<CardComboState>();
    auto forms = AddState<TransformationState>();
    auto battlestart = AddState<BattleStartState>();
    auto battleover = AddState<BattleOverState>();
    auto timeFreeze = AddState<TimeFreezeState>();
    auto reward = AddState<RewardState>();

    // reguster the battle scene with this state
    auto fadeout = AddState<FadeOutState>(FadeOut::black);
    fadeout->bsPtr = this;

    // the macro below is the same as this formal C++ notation...
    /*intro.ChangeOnEvent(cardSelect,     &IntroMobState::IsOver);
    cardSelect.ChangeOnEvent(battlestart, &CardSelectState::OKIsPressed);
    battlestart.ChangeOnEvent(combat,     &BattleStartState::IsFinished);
    battleover.ChangeOnEvent(reward,      &BattleOverState::IsFinished);
    timeFreeze.ChangeOnEvent(combat,      &TimeFreezeState::IsOver);
    reward.ChangeOnEvent(fadeout,         &RewardState::OKIsPressed);*/

    //        hange from       , to         , when this is true
    CHANGE_ON_EVENT(intro      , cardSelect , IsOver);
    CHANGE_ON_EVENT(cardSelect , battlestart, OKIsPressed);
    CHANGE_ON_EVENT(battlestart, combat     , IsFinished);
    CHANGE_ON_EVENT(battleover , reward     , IsFinished);
    CHANGE_ON_EVENT(timeFreeze , combat     , IsOver);
    CHANGE_ON_EVENT(reward     , fadeout    , OKIsPressed);

    // combat has multiple state interruptions based on events
    // so we chain them together instead of using the macro
    combat.ChangeOnEvent(battleover, &CombatState::IsCombatOver)
            .ChangeOnEvent(cardSelect, &CombatState::IsCardGaugeFull)
            .ChangeOnEvent(timeFreeze, &CombatState::HasTimeFreeze);

    // this kicks-off the state graph beginning with the intro state
    this->Start(intro);
}

MobBattleScene::~MobBattleScene() {

}