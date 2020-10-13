#include "bnFadeOutBattleState.h"
#include "../bnBattleSceneBase.h"

FadeOutBattleState::FadeOutBattleState(const FadeOut& mode) : mode(mode) {}

void FadeOutBattleState::onStart() {
  GetScene().Quit(mode);
}