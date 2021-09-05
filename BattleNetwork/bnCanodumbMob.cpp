#include "bnCanodumbMob.h"
#include "bnWebClientMananger.h"
#include "bnCardUUIDs.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnFadeInState.h"

#include "bnAura.h"

Mob* CanodumbMob::Build(Field* field) {
  Mob* mob = new Mob(field);

  // Assign rewards based on rank
  mob->RegisterRankedReward(3, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Cannon_WILD)));
  mob->RegisterRankedReward(11, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Invis_WILD)));

  Battle::Tile* tile = field->GetAt(4, 2);
  if (!tile->IsWalkable()) { tile->SetState(TileState::normal); }

  auto addAura = [](std::shared_ptr<Character> in) {
    auto aura = in->CreateComponent<Aura>(Aura::Type::BARRIER_200, in);
    aura->Persist(true);
  };

  auto spawner = mob->CreateSpawner<Canodumb>();
  spawner.SpawnAt<FadeInState>(5, 2)->Mutate(addAura);
  spawner.SpawnAt<FadeInState>(6, 1)->Mutate(addAura);
  spawner.SpawnAt<FadeInState>(6, 3)->Mutate(addAura);
 
  return mob;
}
