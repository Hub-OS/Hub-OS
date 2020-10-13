#include "bnFadeOutBattleState.h"
#include "../bnBattleSceneBase.h"

FadeOutBattleState::FadeOutBattleState(const FadeOut& mode) : mode(mode) {}

void FadeOutBattleState::onStart() {
  GetScene().Quit(mode);
}

void FadeOutBattleState::onEnd()
{
}

void FadeOutBattleState::onUpdate(double)
{
}

void FadeOutBattleState::onDraw(sf::RenderTexture&)
{
}
