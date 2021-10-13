#include "bnHoneyBomberMob.h"
#include "bnMettaur.h"
#include "bnField.h"
#include "bnCardUUIDs.h"
#include "bnFadeInState.h"

Mob* HoneyBomberMob::Build(Field* field) {
  Mob* mob = new Mob(field);

  auto spawner = mob->CreateSpawner<HoneyBomber>();
  spawner.SpawnAt<FadeInState>(4 + (rand() % 3), 1);
  spawner.SpawnAt<FadeInState>(4 + (rand() % 3), 2);
  spawner.SpawnAt<FadeInState>(4 + (rand() % 3), 3);

  return mob;
}
