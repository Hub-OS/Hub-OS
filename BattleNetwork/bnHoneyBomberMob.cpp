#include "bnHoneyBomberMob.h"
#include "bnMettaur.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnCardsSpawnPolicy.h"

HoneyBomberMob::HoneyBomberMob(Field* field) : MobFactory(field)
{
}


HoneyBomberMob::~HoneyBomberMob()
{
}

Mob* HoneyBomberMob::Build() {
  Mob* mob = new Mob(field);

  mob->Spawn<Rank1<HoneyBomber>>(4 + (rand() % 3), 1);
  mob->Spawn<Rank1<HoneyBomber>>(4 + (rand() % 3), 2);
  mob->Spawn<Rank1<HoneyBomber>>(4 + (rand() % 3), 3);

  return mob;
}
