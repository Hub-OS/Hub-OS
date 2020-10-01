#pragma once
#include "battlescene/bnBattleSceneBaseBase.h"

class Player; //!< Forward declare

// PVP properties the scene needs
struct PVPBattleProperties {
    swoosh::ActivityController* controller{nullptr};
    Player* player;
    int rounds;
};

class PVPBattleScene final : public BattleSceneBase {
    Player* remotePlayer; // the other person

    PVPBattleScene(const PVPBattleProperties& props);

    ~PVPBattleScene();
};
