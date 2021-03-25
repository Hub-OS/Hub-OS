#include "bnFalzarMob.h"
#include "bnSpawnPolicy.h"
#include "bnVirusBackground.h"
#include "bnFalzar.h"

FalzarMob::FalzarMob(Field* field) : MobFactory(field) {

}

FalzarMob::~FalzarMob() {

}

Mob* FalzarMob::Build() {
    Mob* mob = new Mob(field);
    mob->ToggleBossFlag();
    mob->StreamCustomMusic("resources/loops/loop_falzar.ogg");

    auto bg = std::make_shared<VirusBackground>();
    bg->ScrollUp();
    bg->SetScrollSpeed(0.15f); // crawl upward slowly
    mob->SetBackground(bg);

    for(int i = 4; i <= 6; i++) {
        for(int j = 1; j <= 3; j++) {
            Battle::Tile* tile = field->GetAt(i, j);
            tile->SetState(TileState::hidden); // immutable
        }
    }

    mob->Spawn<Rank1<Falzar>>(5, 2);

    return mob;
}