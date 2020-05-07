#include "bnMetalManBossFight.h"
#include "bnMetalMan.h"
#include "bnBattleItem.h"
#include "bnStringEncoder.h"
#include "bnCard.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnSpawnPolicy.h"
#include "bnGear.h"

#include "bnUndernetBackground.h"

MetalManBossFight::MetalManBossFight(Field* field) : MobFactory(field)
{
}


MetalManBossFight::~MetalManBossFight()
{
}

Mob* MetalManBossFight::Build() {
  Mob* mob = new Mob(field);
  mob->SetBackground(new UndernetBackground());
  mob->StreamCustomMusic("resources/loops/loop_boss_battle.ogg");

  //mob->RegisterRankedReward(1, BattleItem(Battle::Card(100, 139, 'Y', 0, Element::none, "YoYo", "", "", 0)));
  //mob->RegisterRankedReward(4, BattleItem(Battle::Card(100, 139, '*', 0, Element::none, "YoYo", "", "", 0)));

  field->AddEntity(*new Gear(field, Team::blue, Direction::left), 3, 2);
  field->AddEntity(*new Gear(field, Team::blue, Direction::right), 4, 2);

  mob->Spawn<Rank1<MetalMan>>(6, 2);

  mob->ToggleBossFlag();

  for (int i = 0; i < field->GetWidth(); i++) {
    for (int j = 0; j < field->GetHeight(); j++) {
      Battle::Tile* tile = field->GetAt(i + 1, j + 1);
    }
  }

  return mob;
}
