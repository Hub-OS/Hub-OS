#include "bnTwoMettaurMob.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnCardsSpawnPolicy.h"

TwoMettaurMob::TwoMettaurMob(Field* field) : MobFactory(field)
{
}


TwoMettaurMob::~TwoMettaurMob()
{
}

Mob* TwoMettaurMob::Build() {
  // Construct a new mob 
  Mob* mob = new Mob(field);

  // Assign rewards based on rank
  //Card name="CrckPanel" cardIndex="83" iconIndex="158" damage="0" type="NONE" codes="K" desc="Cracks a panel" verbose="Cracks the tiles in the column immediately in front" rarity="2" 
  //Card name="Rflctr1" cardIndex="72" iconIndex="99" damage="60" type="NONE" codes="*" desc="Defends and reflects" verbose="Press A to bring up a shield that protects you and reflects damage." rarity="2"
  //mob->RegisterRankedReward(1, BattleItem(Card(72, 99, 'C', 60, Element::NONE, "Rflctr1", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));
  //mob->RegisterRankedReward(1, BattleItem(Card(72, 99, 'B', 60, Element::NONE, "Rflctr1", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));
  //mob->RegisterRankedReward(3, BattleItem(Card(72, 99, 'A', 60, Element::NONE, "Rflctr1", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));
  //mob->RegisterRankedReward(1, BattleItem(Card(83, 158, 'K', 0, Element::NONE, "CrckPanel", "Cracks a panel", "Cracks the tiles in the column immediately in front", 2)));

  int count = 2;

  // place a hole somewhere
  field->GetAt( 4 + (rand() % 3), 1 + (rand() % 3))->SetState(TileState::EMPTY);

  while (count > 0) {
    for (int i = 0; i < field->GetWidth(); i++) {
      for (int j = 0; j < field->GetHeight(); j++) {
        Battle::Tile* tile = field->GetAt(i + 1, j + 1);

        //tile->SetState(TileState(rand()%int(TileState::SIZE)));

        /*if (tile->GetTeam() == Team::RED) {
          if (i == 1 && j == 1) {
            tile->SetState(TileState::DIRECTION_RIGHT);
          }
        }

        if (tile->GetTeam() == Team::BLUE) {
          switch (j) {
          case 0:
            tile->SetState(TileState::DIRECTION_LEFT);
            break;
          case 1:
          {
            if (i == 3)
              tile->SetState(TileState::DIRECTION_DOWN);
            else if (i == 5)
              tile->SetState(TileState::DIRECTION_UP);
          }
            break;
          case 2:
          {
            if (i == 5)
              tile->SetState(TileState::DIRECTION_UP);
            else
              tile->SetState(TileState::DIRECTION_RIGHT);
          }
            break;
          }
        }*/

        if (tile->IsWalkable() && tile->GetTeam() == Team::BLUE) {
          if (rand() % 50 > 25 && count-- > 0)
            mob->Spawn<Rank1<Mettaur>>(i + 1, j + 1);
        }
      }
    }
  }

  return mob;
}
