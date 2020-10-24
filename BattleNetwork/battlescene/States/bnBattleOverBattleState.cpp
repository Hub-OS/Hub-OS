#include "bnBattleOverBattleState.h"
#include "../bnBattleSceneBase.h"

#include "../../bnPlayer.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

#include <Swoosh/Ease.h>

bool BattleOverBattleState::IsFinished() {
  return battleEndTimer.getElapsed().asMilliseconds() >= postBattleLength;
}

BattleOverBattleState::BattleOverBattleState(std::vector<Player*> tracked) : tracked(tracked)
{
  battleEnd = sf::Sprite(*LOAD_TEXTURE(ENEMY_DELETED));
  battleEnd.setOrigin(battleEnd.getLocalBounds().width / 2.0f, battleEnd.getLocalBounds().height / 2.0f);
  battleEnd.setPosition(sf::Vector2f(240.f, 140.f));
  battleEnd.setScale(2.f, 2.f);
}

void BattleOverBattleState::onStart(const BattleSceneState*)
{
  battleEndTimer.reset();
  AUDIO.Stream("resources/loops/enemy_deleted.ogg");

  for (auto p : tracked) {
    auto animComponent = p->GetFirstComponent<AnimationComponent>();

    // If animating, let the animation end to look natural
    if (animComponent) {
      animComponent->OnFinish([p] {
        p->ChangeState<PlayerIdleState>();
      });
    }
    else {
      // Otherwise force state
      p->ChangeState<PlayerIdleState>();
    }
  }

  GetScene().GetField()->RequestBattleStop();
}

void BattleOverBattleState::onEnd(const BattleSceneState*)
{
}

void BattleOverBattleState::onUpdate(double elapsed)
{
  battleEndTimer.update(elapsed);

  // finish whatever animations were happening
  GetScene().GetField()->Update(elapsed);
}

void BattleOverBattleState::onDraw(sf::RenderTexture& surface)
{
  double battleEndSecs = battleEndTimer.getElapsed().asMilliseconds();
  double scale = swoosh::ease::wideParabola(battleEndSecs, postBattleLength, 2.0);
  battleEnd.setScale(2.f, (float)scale * 2.f);

  ENGINE.Draw(battleEnd);
}
