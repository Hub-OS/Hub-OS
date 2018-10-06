#include "bnRandomMettaurMob.h"
#include "bnProgsMan.h"
#include "bnCanodumb.h"
#include "bnCanodumbIdleState.h"
#include "bnBattleItem.h"
#include "bnStringEncoder.h"
#include "bnChip.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnSpawnPolicy.h"

RandomMettaurMob::RandomMettaurMob(Field* field) : MobFactory(field)
{
}


RandomMettaurMob::~RandomMettaurMob()
{
}

Mob* RandomMettaurMob::Build() {
  Mob* mob = new Mob(field);
  mob->RegisterRankedReward(1, BattleItem(Chip(72, 0, '*', 0, Element::NONE, "Reflct", "Defends and reflects", 2)));
  mob->RegisterRankedReward(5, BattleItem(Chip(83, 0, 'K', 0, Element::NONE, "CrckPanel", "Cracks a panel", 2)));

  for (int i = 0; i < field->GetWidth(); i++) {
    for (int j = 0; j < field->GetHeight(); j++) {
      Battle::Tile* tile = field->GetAt(i + 1, j + 1);

      tile->SetState((TileState)(rand() % (int)TileState::EMPTY)); // Make it random excluding an empty tile
      //tile->SetState(TileState::LAVA); 

      if (tile->IsWalkable() && tile->GetTeam() == Team::BLUE) {
        if (rand() % 50 > 30) {
          if (rand() % 10 > 8) {
            mob->Spawn<Rank1<Mettaur, MettaurIdleState>>(i + 1, j + 1);
          }
          else if(rand() % 10 > 5) {
            mob->Spawn<RankSP<Mettaur, MettaurIdleState>>(i + 1, j + 1);
          }
          else if (rand() % 10 > 3) {
            if (rand() % 10 > 8) {
              mob->Spawn<Rank1<Canodumb, CanodumbIdleState>>(i + 1, j + 1);
            }
            else if (rand() % 10 > 5) {
              mob->Spawn<Rank2<Canodumb, CanodumbIdleState>>(i + 1, j + 1);
            }
            else {
              mob->Spawn<Rank3<Canodumb, CanodumbIdleState>>(i + 1, j + 1);
            }
          }
        }
      }
    }
  }

  if (mob->GetMobCount() == 0) {
    int x = (field->GetWidth() / 2) + 1;
    int y = (field->GetHeight() / 2) + 1;
    mob->Spawn<Rank1<Mettaur, MettaurIdleState>>(x, y);

    Battle::Tile* tile = field->GetAt(x, y);
    if (!tile->IsWalkable()) { tile->SetState(TileState::NORMAL); }
  }

  return mob;
}
