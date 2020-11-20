#include "bnBattleStartBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"

BattleStartBattleState::BattleStartBattleState(std::vector<Player*>& tracked) : tracked(tracked) {
  battleStart = sf::Sprite(*LOAD_TEXTURE(BATTLE_START));
  battleStart.setOrigin(battleStart.getLocalBounds().width / 2.0f, battleStart.getLocalBounds().height / 2.0f);
  battleStartPos = sf::Vector2f(240.f, 140.f);
  battleStart.setPosition(battleStartPos);
  battleStart.setScale(2.f, 2.f);
}

void BattleStartBattleState::onStart(const BattleSceneState*)
{
  battleStartTimer.reset();
  battleStartTimer.start();

  for (Player* player : tracked) {
    auto ui = player->GetFirstComponent<SelectedCardsUI>();

    if (ui) {
      ui->Reveal();
    }
  }
}

void BattleStartBattleState::onEnd(const BattleSceneState*)
{
}

void BattleStartBattleState::onUpdate(double elapsed)
{
  battleStartTimer.update(elapsed);
}

void BattleStartBattleState::onDraw(sf::RenderTexture& surface)
{
  double battleStartSecs = battleStartTimer.getElapsed().asMilliseconds();
  double scale = swoosh::ease::wideParabola(battleStartSecs, preBattleLength, 2.0);
  battleStart.setScale(2.f, (float)scale * 2.f);

  ENGINE.Draw(GetScene().GetCardSelectWidget());
  ENGINE.Draw(battleStart);
}

bool BattleStartBattleState::IsFinished() {
  return battleStartTimer.getElapsed().asMilliseconds() >= preBattleLength;
}