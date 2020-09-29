#include "bnMobIntroBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnPlayer.h"
#include "../../bnMob.h"
#include "../../bnAgent.h"

void MobIntroBattleState::onUpdate(double elapsed)
{
  if (mob->NextMobReady()) {
    Mob::MobData* data = mob->GetNextMob();

    Agent* cast = dynamic_cast<Agent*>(data->mob);

    // Some entities have AI and need targets
    // TODO: support multiple targets
    Player* player = tracked[0];
    if (cast) {
      cast->SetTarget(player);
    }

    data->mob->ToggleTimeFreeze(false);
    GetScene().GetField()->AddEntity(*data->mob, data->tileX, data->tileY);

    // Listen for counters
    GetScene().CounterHitListener::Subscribe(*data->mob);
  }

}

const bool MobIntroBattleState::IsOver() {
    return true;
}

MobIntroBattleState::MobIntroBattleState(Mob* mob, std::vector<Player*> tracked) 
  : mob(mob), tracked(tracked)
{
}
