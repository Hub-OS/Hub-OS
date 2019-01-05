#include "bnCanodumbMob.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnSpawnPolicy.h"
#include "bnChipsSpawnPolicy.h"

CanodumbMob::CanodumbMob(Field* field) : MobFactory(field)
{
}


CanodumbMob::~CanodumbMob()
{
}

Mob* CanodumbMob::Build() {
  Mob* mob = new Mob(field);

  Battle::Tile* tile = field->GetAt(4, 2);
  if (!tile->IsWalkable()) { tile->SetState(TileState::NORMAL); }

  mob->Spawn< ChipsSpawnPolicy<Canodumb, CanodumbIdleState> >(5, 2);
  mob->Spawn< Rank2<Canodumb, CanodumbIdleState> >(6, 1);
  mob->Spawn< Rank3<Canodumb, CanodumbIdleState> >(6, 3);

  return mob;
}
