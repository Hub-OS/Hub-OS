#include "bnMobIntroBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnPlayer.h"
#include "../../bnMob.h"
#include "../../bnAgent.h"

MobIntroBattleState::MobIntroBattleState(Mob* mob): 
  mob(mob)
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
      cast->SetTarget(GetScene().GetLocalPlayer());
    }

    std::shared_ptr<Character>& enemy = data->character;

    enemy->ToggleTimeFreeze(false);

    Battle::Tile* destTile = GetScene().GetField()->GetAt(data->tileX, data->tileY);
    enemy->SetTeam(destTile->GetTeam());

    GetScene().GetField()->AddEntity(enemy, data->tileX, data->tileY);

    if (destTile->GetTeam() == Team::red) {
      friendlies.push_back(enemy);
    }

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

  // Untrack friendlies
  for (std::shared_ptr<Character>& f : friendlies) {
    mob->Forget(*f);
  }

  for (std::shared_ptr<Player>& player : GetScene().GetAllPlayers()) {
    player->ChangeState<PlayerControlledState>();
  }
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