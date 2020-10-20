#include "bnFadeOutBattleState.h"
#include "../bnBattleSceneBase.h"

#include "../../bnPlayer.h"

FadeOutBattleState::FadeOutBattleState(const FadeOut& mode, std::vector<Player*> tracked) : mode(mode), tracked(tracked) {}

void FadeOutBattleState::onStart() {
  for (auto p : tracked) {
    p->ChangeState<PlayerIdleState>();
  }

  GetScene().GetField()->RequestBattleStop();
}

void FadeOutBattleState::onEnd()
{
}

void FadeOutBattleState::onUpdate(double elapsed)
{
  wait -= elapsed;

  if (wait <= 0.0) {
    GetScene().Quit(mode);
  }
  else {
    GetScene().GetField()->Update(elapsed);
  }
}

void FadeOutBattleState::onDraw(sf::RenderTexture&)
{
}
