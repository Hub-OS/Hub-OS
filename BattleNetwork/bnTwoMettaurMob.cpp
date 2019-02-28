#include "bnTwoMettaurMob.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnChipsSpawnPolicy.h"

TwoMettaurMob::TwoMettaurMob(Field* field) : MobFactory(field)
{
}


TwoMettaurMob::~TwoMettaurMob()
{
}

Mob* TwoMettaurMob::Build() {
  Mob* mob = new Mob(field);

  mob->RegisterRankedReward(1, BattleItem(Chip(72, 0, '*', 0, Element::NONE, "Rflector", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));
  mob->RegisterRankedReward(3, BattleItem(Chip(83, 0, 'K', 0, Element::NONE, "CrckPanel", "Cracks a panel", "Cracks the tiles in the column immediately in front", 2)));

  int count = 2;

  // place a hole somewhere
  field->GetAt( 4 + (rand() % 3), 1 + (rand() % 3))->SetState(TileState::EMPTY);

  while (count > 0) {
    for (int i = 0; i < field->GetWidth(); i++) {
      for (int j = 0; j < field->GetHeight(); j++) {
        Battle::Tile* tile = field->GetAt(i + 1, j + 1);
        //tile->SetState(TileState::ICE);

        if (tile->IsWalkable() && tile->GetTeam() == Team::BLUE) {
          if (rand() % 50 > 25 && count-- > 0)
            mob->Spawn<Rank1<Mettaur, MettaurIdleState>>(i + 1, j + 1);
        }
      }
    }
  }

  return mob;
}
