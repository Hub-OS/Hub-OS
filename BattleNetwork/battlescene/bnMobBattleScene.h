#pragma once
#include "bnBattleSceneBase.h"

class Player;
class Mob;

/*
    \brief Lots of properties packed into a clean struct
*/
struct MobBattleProperties {
    swoosh::ActivityController* controller{nullptr};
    
    enum class RewardBehavior : int {
        take = 0,    //!< One reward at the end of all mobs
        takeForEach, //!< Each mobs rewards the player
        none         //!< No rewards given
    };

    Player* player;
    std::vector<Mob*> mobs;
    RewardBehavior reward;
};

/*
    \brief Battle scene configuration for a regular PVE battle
*/
class MobBattleScene final : public BattleSceneBase {
    MobBattleProperties props;

    public:
    MobBattleScene(const MobBattleProperties& props);
    ~MobBattleScene();
};