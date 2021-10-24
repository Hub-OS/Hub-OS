#include "bnMetridMob.h"
#include "bnMetrid.h"
#include "bnCanodumb.h"
#include "bnField.h"
#include "bnTile.h"
#include "bnCardUUIDs.h"
#include "bnFadeInState.h"

Mob* MetridMob::Build(Field* field) {
  int mobType = rand() % 3; 

  Mob* mob = new Mob(field);

  // 0 - metrid and cannodumb
  // 1 - 2 metrid and cannodumb of higher types
  // 2 - highest metrids, lava tiles, and holes
  if (mobType == 0) {

    Battle::Tile* tile = field->GetAt(4, 2);
    if (!tile->IsWalkable()) { tile->SetState(TileState::normal); }

    auto spawner = mob->CreateSpawner<Metrid>();
    spawner.SpawnAt<FadeInState>(5, 1);
    spawner.SpawnAt<FadeInState>(6, 2);

    return mob;

  } else if (mobType == 1) {

    Battle::Tile* tile = field->GetAt(2, 1);
    tile->SetState(TileState::empty);

    tile = field->GetAt(6, 3);
    tile->SetState(TileState::empty);

    auto spawner = mob->CreateSpawner<Metrid>();
    spawner.SpawnAt<FadeInState>(5, 1);
    spawner.SpawnAt<FadeInState>(6, 2);

    auto spawner2 = mob->CreateSpawner<Canodumb>();
    spawner2.SpawnAt<FadeInState>(5, 3);

    return mob;
  }

  // else

  for (int i : {0, 5}) {
    for (int j = 0; j < 3; j++) {
      field->GetAt(i + 1, j + 1)->SetState(TileState::volcano);
    }
  }

  Battle::Tile* tile = field->GetAt(1, (rand()%3)+1);
  tile->SetState(TileState::empty);

  if (rand() % 10 < 5) {
    Battle::Tile* tile = field->GetAt(3, (rand() % 3) + 1);
    tile->SetState(TileState::empty);
  }

  tile = field->GetAt(4, 2);
  tile->SetState(TileState::empty);

  tile = field->GetAt(4, 1);
  tile->SetState(TileState::normal);

  auto spawner = mob->CreateSpawner<Metrid>(Metrid::Rank::_3);
  spawner.SpawnAt<FadeInState>(5, 1);
  spawner.SpawnAt<FadeInState>(6, 2);

  auto spawner2 = mob->CreateSpawner<Canodumb>(Canodumb::Rank::_3);
  spawner2.SpawnAt<FadeInState>(4, 1);

  return mob;
}
