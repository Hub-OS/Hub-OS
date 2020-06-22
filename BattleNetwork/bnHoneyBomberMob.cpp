#include "bnHoneyBomberMob.h"
#include "bnMettaur.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnCardsSpawnPolicy.h"
#include "bnWebClientMananger.h"
#include "bnCardUUIDs.h"

HoneyBomberMob::HoneyBomberMob(Field* field) : MobFactory(field)
{
}


HoneyBomberMob::~HoneyBomberMob()
{
}

Mob* HoneyBomberMob::Build() {
  Mob* mob = new Mob(field);

  mob->RegisterRankedReward(1, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Reflect_A)));
  mob->RegisterRankedReward(3, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Tornado_T)));
  mob->RegisterRankedReward(9, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::YoYo_L)));

  mob->Spawn<Rank1<HoneyBomber>>(4 + (rand() % 3), 1);
  mob->Spawn<Rank1<HoneyBomber>>(4 + (rand() % 3), 2);
  mob->Spawn<Rank1<HoneyBomber>>(4 + (rand() % 3), 3);

  return mob;
}
