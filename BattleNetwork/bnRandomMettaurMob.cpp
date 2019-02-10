#include "bnRandomMettaurMob.h"
#include "bnProgsMan.h"
#include "bnMetalMan.h"
#include "bnCanodumb.h"
#include "bnCanodumbIdleState.h"
#include "bnBattleItem.h"
#include "bnStringEncoder.h"
#include "bnChip.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnSpawnPolicy.h"
#include "bnMysteryData.h"
#include "bnGear.h"
#include "bnBattleOverTrigger.h"

RandomMettaurMob::RandomMettaurMob(Field* field) : MobFactory(field)
{
}


RandomMettaurMob::~RandomMettaurMob()
{
}

Mob* RandomMettaurMob::Build() {
  Mob* mob = new Mob(field);
  mob->RegisterRankedReward(1, BattleItem(Chip(72, 0, '*', 0, Element::NONE, "Reflct", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));
  mob->RegisterRankedReward(5, BattleItem(Chip(83, 0, 'K', 0, Element::NONE, "CrckPanel", "Cracks a panel", "Cracks the tiles in the column immediately in front", 2)));


  field->OwnEntity(new Gear(field, Team::BLUE, Direction::LEFT), 3, 2);
  field->OwnEntity(new Gear(field, Team::BLUE, Direction::RIGHT), 4, 2);

  mob->Spawn<Rank1<MetalMan, MetalManIdleState>>(6, 2);

  mob->ToggleBossFlag();

  bool AllIce = (rand() % 10 > 5);
  for (int i = 0; i < field->GetWidth(); i++) {
    for (int j = 0; j < field->GetHeight(); j++) {
      Battle::Tile* tile = field->GetAt(i + 1, j + 1);

      /*tile->SetState((TileState)(rand() % (int)TileState::EMPTY)); // Make it random excluding an empty tile

      if (AllIce) {
        tile->SetState(TileState::ICE);
      }*/

      /*if (tile->IsWalkable() && tile->GetTeam() == Team::BLUE && !tile->ContainsEntityType<Character>()) {
        if (rand() % 50 > 30) {
          if (rand() % 10 > 5) {
            MysteryData* mystery = new MysteryData(mob->GetField(), Team::UNKNOWN);
            field->OwnEntity(mystery, tile->GetX(), tile->GetY());

            auto callback = [](BattleScene& b, MysteryData& m) {
              m.RewardPlayer();
            };

            Component* c = new BattleOverTrigger<MysteryData>(mystery, callback);

            mob->AddComponent(c);
          }
          else if(rand() % 10 > 5) {
            mob->Spawn<Rank1<Mettaur, MettaurIdleState>>(i + 1, j + 1);
          }
          else if (rand() % 10 > 3) {
            if (rand() % 10 > 8) {
              mob->Spawn<Rank1<Canodumb, CanodumbIdleState>>(i + 1, j + 1);
            }
            else if (rand() % 10 > 10) {
              mob->Spawn<Rank2<Canodumb, CanodumbIdleState>>(i + 1, j + 1);
            }
            else {
              mob->Spawn<Rank3<Canodumb, CanodumbIdleState>>(i + 1, j + 1);
            }
          }
          else if (rand() % 10 > 1) {
            mob->Spawn<Rank1<ProgsMan, ProgsManIdleState>>(i + 1, j + 1);
          }
        }
      }*/
    }
  }

  if (mob->GetMobCount() == 0) {
    int x = (field->GetWidth() / 2) + 2;
    int y = (field->GetHeight() / 2) + 1;
    mob->Spawn<Rank1<Mettaur, MettaurIdleState>>(x, y);

    Battle::Tile* tile = field->GetAt(x, y);
    if (!tile->IsWalkable() && !tile->ContainsEntityType<Character>()) { tile->SetState(TileState::NORMAL); }
  }

  return mob;
}
