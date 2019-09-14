#include "bnAlphaBossFight.h"
#include "bnAlphaCore.h"
#include "bnBattleItem.h"
#include "bnStringEncoder.h"
#include "bnChip.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"
#include "bnFadeInState.h"

AlphaBossFight::AlphaBossFight(Field* field) : MobFactory(field)
{
}


AlphaBossFight::~AlphaBossFight()
{
}

Mob* AlphaBossFight::Build() {
  Mob* mob = new Mob(field);

  // Changes music and ranking algorithm
  mob->ToggleBossFlag();
  mob->StreamCustomMusic("resources/loops/proto.ogg");

  mob->Spawn<Rank1<AlphaCore, FadeInState>>(5, 2);

  Battle::Tile* tile = field->GetAt(5, 2);
  if (!tile->IsWalkable()) { tile->SetState(TileState::NORMAL); }

  return mob;
}
