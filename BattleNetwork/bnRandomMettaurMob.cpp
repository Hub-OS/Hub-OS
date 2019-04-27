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
#include "bnChipsSpawnPolicy.h"
#include "bnMysteryData.h"
#include "bnGear.h"
#include "bnBattleOverTrigger.h"
#include "bnUndernetBackground.h"

RandomMettaurMob::RandomMettaurMob(Field* field) : MobFactory(field)
{
}


RandomMettaurMob::~RandomMettaurMob()
{
}

Mob* RandomMettaurMob::Build() {
  // Build a mob around the field input
  Mob* mob = new Mob(field);

  // Add a ranked reward
  mob->RegisterRankedReward(3, BattleItem(Chip(82, 154, '*', 0, Element::NONE, "AreaGrab", "Defends and reflects", "Press A to bring up a shield that protects you and reflects damage.", 2)));

  // Randomly set the stage to all ice
  bool AllIce = (rand() % 50 > 45);
  
  // Keep track of mystery objects spawned
  int mysterycount = 0;

  // Spawn at least one enemy
  while (mob->GetMobCount() == 0) {
    // Scan across the grid
    for (int i = 0; i < field->GetWidth(); i++) {
      for (int j = 0; j < field->GetHeight(); j++) {
        // Get the tile
        Battle::Tile* tile = field->GetAt(i + 1, j + 1);

        // If the stage is all ice, set the tile to ICE
        if (AllIce) { tile->SetState(TileState::ICE); }

        // If the tile is walkable, free, and BLUE team, spawn an enemy
        if (tile->IsWalkable() && tile->GetTeam() == Team::BLUE && !tile->ContainsEntityType<Character>() && !tile->ContainsEntityType<MysteryData>()) {
          if (rand() % 50 > 30) {
            // Randomly spawn a mystery data object (at max: 3)
            if (rand() % 100 > 90 && mysterycount < 3) {
              MysteryData* mystery = new MysteryData(mob->GetField(), Team::UNKNOWN);
              field->AddEntity(*mystery, tile->GetX(), tile->GetY());

              // Callback for the battle over trigger
              auto callback = [](BattleScene& b, MysteryData& m) {
                m.RewardPlayer();
              };

              // Create a trigger for the mysterydata
              Component* c = new BattleOverTrigger<MysteryData>(mystery, callback);

              // Pass the component off to the battle scene to own
              mob->DelegateComponent(c);

              mysterycount++;
            }
            else if (rand() % 10 > 0) {
              if (rand() % 10 > 5) {
                mob->Spawn<Rank1<MetalMan, MetalManIdleState>>(i + 1, j + 1);
              }
              else {
                // Spawn metal man with chips
                mob->Spawn<ChipsSpawnPolicy<MetalMan, MetalManIdleState>>(i + 1, j + 1);
              }
            }
            else if (rand() % 10 > 3) {
              if (rand() % 10 > 8) {
                mob->Spawn<Rank1<Canodumb, CanodumbIdleState>>(i + 1, j + 1);
              }
              else if (rand() % 10 > 4) {
                mob->Spawn<Rank2<Canodumb, CanodumbIdleState>>(i + 1, j + 1);
              }
            }
            else if (rand() % 100 < 10) {
              if (rand() % 10 > 5) {
                mob->Spawn<Rank1<ProgsMan, ProgsManIdleState>>(i + 1, j + 1);
              }
              else {
                // Spawn progsman with chips
                mob->Spawn<ChipsSpawnPolicy<ProgsMan, ProgsManIdleState>>(i + 1, j + 1);
              }
            }
            else if (rand() % 1000 == 889) {
              mob->Spawn<RankEX<MetalMan, MetalManIdleState>>(i + 1, j + 1);
            }
          }
        }
      }
    }
  }

  return mob;
}
