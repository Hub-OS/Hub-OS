#include "bnMetalManBossFight2.h"
#include "bnMetalMan.h"
#include "bnBattleItem.h"
#include "bnStringEncoder.h"
#include "bnCard.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnSpawnPolicy.h"
#include "bnGear.h"

#include "bnUndernetBackground.h"

MetalManBossFight2::MetalManBossFight2(Field* field) : MobFactory(field)
{
}


MetalManBossFight2::~MetalManBossFight2()
{
}

Mob* MetalManBossFight2::Build() {
  Mob* mob = new Mob(field);
  mob->SetBackground(new UndernetBackground());
  mob->StreamCustomMusic("resources/loops/loop_boss_battle2.ogg");

  //mob->RegisterRankedReward(1, BattleItem(Card(139, 0, '*', 120, Element::NONE, "ProtoMan", "Slices all enmy on field", "ProtoMan appears, stopping time, and teleports to each enemy striking once.", 5)));

  field->AddEntity(*new Gear(field, Team::BLUE, Direction::LEFT), 3, 2);
  field->AddEntity(*new Gear(field, Team::BLUE, Direction::RIGHT), 4, 2);

  mob->Spawn<RankEX<MetalMan>>(6, 2);

  mob->ToggleBossFlag();

  for (int i = 0; i < field->GetWidth(); i++) {
    for (int j = 0; j < field->GetHeight(); j++) {
      Battle::Tile* tile = field->GetAt(i + 1, j + 1);
      tile->SetState(TileState::ICE);
    }
  }

  return mob;
}
