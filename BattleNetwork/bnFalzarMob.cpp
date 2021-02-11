#include "bnFalzarMob.h"
#include "bnVirusBackground.h"
#include "bnFadeInState.h"
#include "bnFalzar.h"

FalzarMob::FalzarMob(Field* field) : MobFactory(field) {

}

FalzarMob::~FalzarMob() {

}

Mob* FalzarMob::Build() {
    Mob* mob = new Mob(field);
    mob->ToggleBossFlag();
    mob->StreamCustomMusic("resources/loops/loop_falzar.ogg");

    auto bg = new VirusBackground();
    bg->ScrollUp();
    bg->SetScrollSpeed(0.15f); // crawl upward slowly
    mob->SetBackground(bg);

    for(int i = 4; i <= 6; i++) {
        for(int j = 1; j <= 3; j++) {
            Battle::Tile* tile = field->GetAt(i, j);
            tile->SetState(TileState::hidden); // immutable
        }
    }

    auto spawner = mob->CreateSpawner<Falzar>();
    spawner.SpawnAt<FadeInState>(5, 2);

    return mob;
}