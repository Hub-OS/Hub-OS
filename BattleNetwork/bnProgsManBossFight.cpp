#include "bnProgsManBossFight.h"
#include "bnProgsMan.h"
#include "bnBattleItem.h"
#include "bnStringEncoder.h"
#include "bnChip.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"

ProgsManBossFight::ProgsManBossFight(Field* field) : MobFactory(field)
{
}


ProgsManBossFight::~ProgsManBossFight()
{
}

Mob* ProgsManBossFight::Build() {
  Mob* mob = new Mob(field);
  mob->ToggleBossFlag();
  mob->RegisterRankedReward(1, BattleItem(Chip(135, 0, 'P', 300, Element::BREAK, "ProgsMan", "Throws ProgBomb", 5)));
  mob->RegisterRankedReward(11, BattleItem(Chip(136, 0, 'P', 300, Element::BREAK, EX("ProgsMan"), "Throws ProgBomb", 5)));

  int x = (field->GetWidth() / 2) + 1;
  int y = (field->GetHeight() / 2) + 1;
  mob->Spawn<RankEX<ProgsMan, ProgsManIdleState>>(x, y);

  Battle::Tile* tile = field->GetAt(x, y);
  if (!tile->IsWalkable()) { tile->SetState(TileState::NORMAL); }

  return mob;
}
