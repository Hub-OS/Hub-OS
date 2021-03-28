#include "bnFadeOutBattleState.h"
#include "../bnBattleSceneBase.h"

#include "../../bnPlayer.h"
#include "../../bnField.h"

FadeOutBattleState::FadeOutBattleState(const FadeOut& mode, std::vector<Player*>& tracked) : mode(mode), tracked(tracked) {}

void FadeOutBattleState::onStart(const BattleSceneState*) {
  for (auto p : tracked) {
    p->ChangeState<PlayerIdleState>();
  }

  GetScene().GetField()->RequestBattleStop();
}

void FadeOutBattleState::onEnd(const BattleSceneState*)
{
}

void FadeOutBattleState::onUpdate(double elapsed)
{
  wait -= elapsed;

  if (wait <= 0.0) {
    GetScene().Quit(mode);
  }
  else if(keepPlaying) {
    GetScene().GetField()->Update(elapsed);
  }
}

void FadeOutBattleState::onDraw(sf::RenderTexture&)
{
}

void FadeOutBattleState::EnableKeepPlaying(bool enable)
{
  this->keepPlaying = enable;
}
