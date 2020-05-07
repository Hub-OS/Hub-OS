#include "bnCanodumbMob.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnSpawnPolicy.h"
#include "bnCardsSpawnPolicy.h"

CanodumbMob::CanodumbMob(Field* field) : MobFactory(field)
{
}


CanodumbMob::~CanodumbMob()
{
}

Mob* CanodumbMob::Build() {
  Mob* mob = new Mob(field);

  Battle::Tile* tile = field->GetAt(4, 2);
  if (!tile->IsWalkable()) { tile->SetState(TileState::normal); }

  mob->Spawn< Rank1<Canodumb> >(5, 2);
  mob->Spawn< Rank2<Canodumb> >(6, 1);
  mob->Spawn< Rank3<Canodumb> >(6, 3);

  return mob;
}
