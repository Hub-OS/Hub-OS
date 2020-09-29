#include "bnCombatBattleState.h"

#include "../bnBattleSceneBase.h"

#include "../../bnTeam.h"
#include "../../bnEntity.h"
#include "../../bnCharacter.h"

const bool CombatBattleState::HasTimeFreeze() const {
    return true;
}

const bool CombatBattleState::IsCombatOver() const {
    return true;
}

const bool CombatBattleState::IsCardGaugeFull() {
    return true;
}

void CombatBattleState::onUpdate(double elapsed)
{
  GetScene().GetField()->Update((float)elapsed);

  auto blueTeamChars = GetScene().GetField()->FindEntities([](Entity* e) {
    return e->GetTeam() == Team::blue && dynamic_cast<Character*>(e);
    });

  if (mob->IsCleared()) {
    field->RequestBattleStop();
    cardUI.DropSubscribers();
  }

  // Check if entire mob is deleted
  if (mob->IsCleared() && blueTeamChars.empty()) {
    if (!isPostBattle && battleEndTimer.getElapsed().asSeconds() < postBattleLength) {
      // Show Enemy Deleted

    }
    else if (!isBattleRoundOver && battleEndTimer.getElapsed().asSeconds() > postBattleLength) {
      isMobDeleted = true;
    }
  }
  else if (!isPlayerDeleted && mob->NextMobReady() && isSceneInFocus) {
    Mob::MobData* data = mob->GetNextMob();

    Agent* cast = dynamic_cast<Agent*>(data->mob);

    // Some entities have AI and need targets
    if (cast) {
      cast->SetTarget(player);
    }

    data->mob->ToggleTimeFreeze(false);
    field->AddEntity(*data->mob, data->tileX, data->tileY);
    mobNames.push_back(data->mob->GetName());

    // Listen for counters
    CounterHitListener::Subscribe(*data->mob);
  }
}
