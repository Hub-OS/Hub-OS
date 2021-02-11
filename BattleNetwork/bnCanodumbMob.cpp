#include "bnCanodumbMob.h"
#include "bnWebClientMananger.h"
#include "bnCardUUIDs.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnFadeInState.h"

CanodumbMob::CanodumbMob(Field* field) : MobFactory(field)
{
}


CanodumbMob::~CanodumbMob()
{
}

Mob* CanodumbMob::Build() {
  Mob* mob = new Mob(field);

  // Assign rewards based on rank
  mob->RegisterRankedReward(3, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Cannon_WILD)));
  mob->RegisterRankedReward(11, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Invis_WILD)));

  Battle::Tile* tile = field->GetAt(4, 2);
  if (!tile->IsWalkable()) { tile->SetState(TileState::normal); }

  auto spawner = mob->CreateSpawner<Canodumb>();
  spawner.SpawnAt<FadeInState>(5, 2);
  spawner.SpawnAt<FadeInState>(6, 1);
  spawner.SpawnAt<FadeInState>(6, 3);

  return mob;
}
