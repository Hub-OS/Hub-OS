#include "bnHoneyBomberMob.h"
#include "bnMettaur.h"
#include "bnField.h"
#include "bnWebClientMananger.h"
#include "bnCardUUIDs.h"
#include "bnFadeInState.h"

Mob* HoneyBomberMob::Build(Field* field) {
  Mob* mob = new Mob(field);

  mob->RegisterRankedReward(1, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Reflect_A)));
  mob->RegisterRankedReward(3, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Tornado_T)));
  mob->RegisterRankedReward(9, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::YoYo_L)));

  auto spawner = mob->CreateSpawner<HoneyBomber>();
  spawner.SpawnAt<FadeInState>(4 + (rand() % 3), 1);
  spawner.SpawnAt<FadeInState>(4 + (rand() % 3), 2);
  spawner.SpawnAt<FadeInState>(4 + (rand() % 3), 3);

  return mob;
}
