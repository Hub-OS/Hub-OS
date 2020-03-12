#include "bnMetridMob.h"
#include "bnMetrid.h"
#include "bnCanodumb.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnSpawnPolicy.h"
#include "bnCardsSpawnPolicy.h"

MetridMob::MetridMob(Field* field) : MobFactory(field)
{
}


MetridMob::~MetridMob()
{
}

Mob* MetridMob::Build() {
  int mobType = rand() % 3; 

  // 0 - metrid and cannodumb
  // 1 - 2 metrid and cannodumb of higher types
  // 2 - highest metrids, lava tiles, and holes
  Mob* mob = new Mob(field);
  //mob->RegisterRankedReward(9, BattleItem(Card(8, 19, '*', 150, Element::FIRE, "FireBrn3", "Crcks 3 sqrs ahd with fire", "This card does not have extra information.", 4)));
  //mob->RegisterRankedReward(6, BattleItem(Card(8, 19, 'A', 150, Element::FIRE, "FireBrn3", "Crcks 3 sqrs ahd with fire", "This card does not have extra information.", 4)));
  //mob->RegisterRankedReward(4, BattleItem(Card(7, 18, 'A', 110, Element::FIRE, "FireBrn2", "Crcks 3 sqrs ahd with fire", "This card does not have extra information.", 4)));
  //mob->RegisterRankedReward(1, BattleItem(Card(7, 18, 'Y', 110, Element::FIRE, "FireBrn2", "Crcks 3 sqrs ahd with fire", "This card does not have extra information.", 4)));

  if (mobType == 0) {

    Battle::Tile* tile = field->GetAt(4, 2);
    if (!tile->IsWalkable()) { tile->SetState(TileState::NORMAL); }

    mob->Spawn< Rank1<Metrid> >(5, 1);
    mob->Spawn< Rank1<Metrid> >(6, 2);

    return mob;

  } else if (mobType == 1) {

    Battle::Tile* tile = field->GetAt(2, 1);
    tile->SetState(TileState::EMPTY);

    tile = field->GetAt(6, 3);
    tile->SetState(TileState::EMPTY);

    mob->Spawn< Rank1<Metrid> >(5, 1);
    mob->Spawn< Rank2<Metrid> >(6, 2);
    mob->Spawn< Rank2<Canodumb> >(5, 3);

    return mob;
  }

  // else

  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 3; j++) {
      field->GetAt(i + 1, j + 1)->SetState(TileState::VOLCANO);
    }
  }

  Battle::Tile* tile = field->GetAt(1, (rand()%3)+1);
  tile->SetState(TileState::EMPTY);

  if (rand() % 10 < 5) {
    Battle::Tile* tile = field->GetAt(3, (rand() % 3) + 1);
    tile->SetState(TileState::EMPTY);
  }

  tile = field->GetAt(4, 2);
  tile->SetState(TileState::EMPTY);

  tile = field->GetAt(4, 1);
  tile->SetState(TileState::NORMAL);

  mob->Spawn< Rank3<Canodumb> >(4, 1);
  mob->Spawn< Rank3<Metrid> >(5, 1);
  mob->Spawn< Rank3<Metrid> >(6, 2);

  return mob;
}
