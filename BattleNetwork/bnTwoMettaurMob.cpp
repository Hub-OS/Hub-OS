#include "bnTwoMettaurMob.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnCardsSpawnPolicy.h"
#include "bnWebClientMananger.h"
#include "bnCardUUIDs.h"

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
  mob->RegisterRankedReward(1, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Reflect_A)));
  mob->RegisterRankedReward(6, BattleItem(WEBCLIENT.MakeBattleCardFromWebCardData(BuiltInCards::Cannon_A)));

  //mob->RegisterRankedReward(1, BattleItem(Battle::Card(72, 99, 'C', 60, Element::none, "Rflctr1", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));
  //mob->RegisterRankedReward(1, BattleItem(Battle::Card(72, 99, 'B', 60, Element::none, "Rflctr1", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));
  //mob->RegisterRankedReward(3, BattleItem(Battle::Card(72, 99, 'A', 60, Element::none, "Rflctr1", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));
  //mob->RegisterRankedReward(1, BattleItem(Battle::Card(83, 158, 'K', 0, Element::none, "CrckPanel", "Cracks a panel", "Cracks the tiles in the column immediately in front", 2)));

  int count = 3;

  // place a hole somewhere
  field->GetAt( 4 + (rand() % 3), 1 + (rand() % 3))->SetState(TileState::empty);

  while (count > 0) {
    for (int i = 0; i < field->GetWidth(); i++) {
      for (int j = 0; j < field->GetHeight(); j++) {
        Battle::Tile* tile = field->GetAt(i + 1, j + 1);

        //tile->SetState(TileState(rand()%int(TileState::size)));

        /*if (tile->GetTeam() == Team::red) {
          if (i == 1 && j == 1) {
            tile->SetState(TileState::directionRight);
          }
        }

        if (tile->GetTeam() == Team::blue) {
          switch (j) {
          case 0:
            tile->SetState(TileState::directionLeft);
            break;
          case 1:
          {
            if (i == 3)
              tile->SetState(TileState::directionDown);
            else if (i == 5)
              tile->SetState(TileState::directionUp);
          }
            break;
          case 2:
          {
            if (i == 5)
              tile->SetState(TileState::directionUp);
            else
              tile->SetState(TileState::directionRight);
          }
            break;
          }
        }*/

        if (tile->IsWalkable() && !tile->IsReservedByCharacter() && tile->GetTeam() == Team::blue) {
          if (rand() % 50 > 25 && count-- > 0)
            mob->Spawn<Rank1<Mettaur>>(i + 1, j + 1);
        }
      }
    }
  }

  return mob;
}
