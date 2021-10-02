#include "bnMobIntroBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnPlayer.h"
#include "../../bnMob.h"
#include "../../bnAgent.h"

MobIntroBattleState::MobIntroBattleState(Mob* mob, std::vector<std::shared_ptr<Player>> tracked): 
  mob(mob), tracked(tracked)
{
}

void MobIntroBattleState::onUpdate(double elapsed)
{
  if (!GetScene().IsSceneInFocus()) return;

  Field& field = *GetScene().GetField();

  if (mob->NextMobReady()) {
    auto data = mob->GetNextSpawn();

    Agent* cast = dynamic_cast<Agent*>(data->character.get());

    // Some entities have AI and need targets
    // TODO: support multiple targets
    if (cast) {
      cast->SetTarget(tracked[0]);
    }

    auto& enemy = data->character;

    enemy->ToggleTimeFreeze(false);
    GetScene().GetField()->AddEntity(enemy, data->tileX, data->tileY);

    // Listen for events
    GetScene().CounterHitListener::Subscribe(*enemy);
    GetScene().HitListener::Subscribe(*enemy);
    GetScene().SubscribeToCardActions(*enemy);
  }

  GetScene().GetField()->Update((float)elapsed);
}

void MobIntroBattleState::onEnd(const BattleSceneState*)
{
  mob->DefaultState();

  for (auto& player : tracked) {
    player->ChangeState<PlayerControlledState>();
  }

  // Tell everything to begin battle
  GetScene().BroadcastBattleStart();
}

void MobIntroBattleState::onStart(const BattleSceneState*)
{
}

void MobIntroBattleState::onDraw(sf::RenderTexture& surface)
{
  surface.draw(GetScene().GetCardSelectWidget());
}

const bool MobIntroBattleState::IsOver() {
  return mob->IsSpawningDone();
}