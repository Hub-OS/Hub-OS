#include "bnFadeOutBattleState.h"
#include "../battlescene/bnBattleSceneBaseBase.h"

FadeOutBattleState::FadeOutBattleState(const FadeOut& mode) : mode(mode) {}

void FadeOutBattleState::onStart() {
  GetScene().Quit(mode);
}