#include "bnTwoMettaurMob.h"
#include "bnField.h"
#include "bnSpawnPolicy.h"

TwoMettaurMob::TwoMettaurMob(Field* field) : MobFactory(field)
{
}


TwoMettaurMob::~TwoMettaurMob()
{
}

Mob* TwoMettaurMob::Build() {
  Mob* mob = new Mob(field);
  int count = 2;

  while (count > 0) {
    for (int i = 0; i < field->GetWidth(); i++) {
      for (int j = 0; j < field->GetHeight(); j++) {
        Battle::Tile* tile = field->GetAt(i + 1, j + 1);
        if (tile->IsWalkable() && tile->GetTeam() == Team::BLUE) {
          if (rand() % 50 > 25 && count-- > 0)
            mob->Spawn<Rank1<Mettaur, MettaurIdleState>>(i + 1, j + 1);
        }
      }
    }
  }

  return mob;
}
