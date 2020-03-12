#include "bnProgsManBossFight.h"
#include "bnProgsMan.h"
#include "bnBattleItem.h"
#include "bnStringEncoder.h"
#include "bnCard.h"
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
  
  // Changes music and ranking algorithm
  mob->ToggleBossFlag();

  ////mob->RegisterRankedReward(1, BattleItem(Card(135, 232, 'P', 300, Element::BREAK, "ProgsMan", "Throws ProgBomb", "Throws a projectile at the nearest enemy.", 5)));
  ////mob->RegisterRankedReward(11, BattleItem(Card(136, 232, 'P', 300, Element::BREAK, EX("ProgsMan"), "Throws ProgBomb", "Throws 3 projectiles at the nearest enemy in successsion.", 5)));

  //mob->RegisterRankedReward(1, BattleItem(Card(100, 139, 'Y', 0, Element::NONE, "YoYo", "", "", 0)));
  //mob->RegisterRankedReward(4, BattleItem(Card(100, 139, '*', 0, Element::NONE, "YoYo", "", "", 0)));

  int x = 5;
  int y = (field->GetHeight() / 2) + 1;
  
  mob->Spawn<RankEX<ProgsMan>>(x, y);

  Battle::Tile* tile = field->GetAt(x, y);
  if (!tile->IsWalkable()) { tile->SetState(TileState::NORMAL); }


  return mob;
}
