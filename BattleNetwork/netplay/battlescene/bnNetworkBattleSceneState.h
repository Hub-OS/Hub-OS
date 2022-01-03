#pragma once

#include <assert.h>

#include "../../battlescene/bnBattleSceneState.h"
#include "bnNetworkBattleScene.h"

using namespace swoosh;

/*
    Interface for Battle Scene States
*/
struct NetworkBattleSceneState : public BattleSceneState {
    friend class NetworkBattleSceneBase;

    NetworkBattleScene& GetScene() { NetworkBattleScene* scenePtr = dynamic_cast<NetworkBattleScene*>(scene); assert(scenePtr); return *scenePtr; }
    const NetworkBattleScene& GetScene() const { NetworkBattleScene* scenePtr = dynamic_cast<NetworkBattleScene*>(scene); assert(scenePtr); return *scenePtr; }

    virtual ~NetworkBattleSceneState(){};
};