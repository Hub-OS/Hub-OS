#include "bnMobIntroBattleState.h"

#include "../battlescene/bnBattleSceneBaseBase.h"
#include "../../bnPlayer.h"
#include "../../bnMob.h"
#include "../../bnAgent.h"

void MobIntroBattleState::onUpdate(double elapsed)
{
  Field& field = *GetScene().GetField();

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
  else if (mob->IsSpawningDone()) {
    for (Player* player : tracked) {
      // allow the player to be controlled by keys
      player->ChangeState<PlayerControlledState>();
    }

    // Move mob characters out of their intro states
    mob->DefaultState();

    // Tell everything to begin battle
    GetScene().BroadcastBattleStart();
  }
}

const bool MobIntroBattleState::IsOver() {
    return mob->IsSpawningDone();
}

MobIntroBattleState::MobIntroBattleState(Mob* mob, std::vector<Player*> tracked) 
  : mob(mob), tracked(tracked)
{
}
