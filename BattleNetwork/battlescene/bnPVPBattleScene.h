#include "bnIBattleScene.h"

class Player; //!< Forward declare

// PVP properties the scene needs
struct PVPBattleProperties {
    swoosh::ActivityController* controller{nullptr};
    Player* player;
    int rounds;
};

class PVPBattleScene final : public IBattleScene {
    Player* remotePlayer; // the other person

    PVPBattleScene(const PVPBattleProperties& props);

    ~PVPBattleScene();
};
