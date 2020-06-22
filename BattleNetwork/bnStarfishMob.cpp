#include "bnStarfishMob.h"
#include "bnMettaur.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnCardsSpawnPolicy.h"
#include "bnWebClientMananger.h"
#include "bnCardUUIDs.h"

StarfishMob::StarfishMob(Field* field) : MobFactory(field)
{
}


StarfishMob::~StarfishMob()
{
}

Mob* StarfishMob::Build() {
  Mob* mob = new Mob(field);
  mob->RegisterRankedReward(1, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::YoYo_M)));

  mob->Spawn<Rank1<Starfish>>(4 + (rand() % 3), 1);
  mob->Spawn<Rank1<Starfish>>(4 + (rand() % 3), 3);

  bool allIce = !(rand() % 10);

  for (auto t : field->FindTiles([](Battle::Tile* t) { return true; })) {
    if (allIce) {
      t->SetState(TileState::ice);
    }
    else {
      if (t->GetX() == 3) {
        if (t->GetY() == 1 || t->GetY() == 3) {
          t->SetState(TileState::directionLeft);
        }
      }
    }
  }

  return mob;
}
