#include "bnFadeOutBattleState.h"

FadeOutBattleState::FadeOutBattleState(IBattleScene* bsPtr, const FadeOut& mode) : bsPtr(bsPtr), mode(mode) {}

void FadeOutBattleState::onStart() {
    if(bsPtr) {
        bsPtr->Quit(mode);
    }
}