#include "bnStarfishMob.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnChipsSpawnPolicy.h"

StarfishMob::StarfishMob(Field* field) : MobFactory(field)
{
}


StarfishMob::~StarfishMob()
{
}

Mob* StarfishMob::Build() {
  Mob* mob = new Mob(field);

  mob->Spawn<Rank1<Starfish, StarfishIdleState>>(4 + (rand() % 3), 1);
  mob->Spawn<Rank1<Starfish, StarfishIdleState>>(4 + (rand() % 3), 3);

  return mob;
}
