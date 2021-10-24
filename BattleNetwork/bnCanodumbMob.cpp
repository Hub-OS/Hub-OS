#include "bnCanodumbMob.h"
#include "bnCardUUIDs.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnFadeInState.h"

#include "bnAura.h"

Mob* CanodumbMob::Build(Field* field) {
  Mob* mob = new Mob(field);

  Battle::Tile* tile = field->GetAt(4, 2);
  if (!tile->IsWalkable()) { tile->SetState(TileState::normal); }

  auto addAura = [](Character& in) {
    auto* aura = in.CreateComponent<Aura>(Aura::Type::BARRIER_200, &in);
    aura->Persist(true);
  };

  auto spawner = mob->CreateSpawner<Canodumb>();
  spawner.SpawnAt<FadeInState>(5, 2)->Mutate(addAura);
  spawner.SpawnAt<FadeInState>(6, 1)->Mutate(addAura);
  spawner.SpawnAt<FadeInState>(6, 3)->Mutate(addAura);
 
  return mob;
}
