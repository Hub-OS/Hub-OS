#include "bnBattleOverBattleState.h"

#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

#include <Swoosh/Ease.h>

bool BattleOverBattleState::IsFinished() {
  return battleEndTimer.getElapsed().asMilliseconds() >= postBattleLength;
}

BattleOverBattleState::BattleOverBattleState()
{
  battleEnd = sf::Sprite(*LOAD_TEXTURE(ENEMY_DELETED));
  battleEnd.setOrigin(battleEnd.getLocalBounds().width / 2.0f, battleEnd.getLocalBounds().height / 2.0f);
  battleEnd.setPosition(sf::Vector2f(240.f, 140.f));
  battleEnd.setScale(2.f, 2.f);
}

void BattleOverBattleState::onStart()
{
  battleEndTimer.reset();
  AUDIO.Stream("resources/loops/enemy_deleted.ogg");
}

void BattleOverBattleState::onEnd()
{
}

void BattleOverBattleState::onUpdate(double elapsed)
{
  battleEndTimer.update(elapsed);
}

void BattleOverBattleState::onDraw(sf::RenderTexture& surface)
{
  double battleEndSecs = battleEndTimer.getElapsed().asMilliseconds();
  double scale = swoosh::ease::wideParabola(battleEndSecs, postBattleLength, 2.0);
  battleEnd.setScale(2.f, (float)scale * 2.f);

  surface.draw(battleEnd);
}
