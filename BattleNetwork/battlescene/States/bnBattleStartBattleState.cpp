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

void BattleStartBattleState::SetStartupDelay(long long microseconds)
{
  startupDelay = microseconds;
}

void BattleStartBattleState::onStart(const BattleSceneState*)
{
  battleStartTimer.reset();
  battleStartTimer.start();

  /*for (Player* player : tracked) {
    auto ui = player->GetFirstComponent<SelectedCardsUI>();

    if (ui) {
      ui->Reveal();
    }
  }*/
}

void BattleStartBattleState::onEnd(const BattleSceneState*)
{
}

void BattleStartBattleState::onUpdate(double elapsed)
{
  battleStartTimer.update(sf::seconds(static_cast<float>(elapsed)));
}

void BattleStartBattleState::onDraw(sf::RenderTexture& surface)
{
  long long start = battleStartTimer.getElapsed().asMicroseconds();

  if (start >= startupDelay) {
    constexpr float micro2sec = 1000000.f;
    float delta = (start - startupDelay) / micro2sec; // cast delta to seconds as floating point num
    float length = preBattleLength / micro2sec;

    float scale = swoosh::ease::wideParabola(delta, length, 2.f);
    battleStart.setScale(2.f, scale * 2.f);

    surface.draw(GetScene().GetCardSelectWidget());
    surface.draw(battleStart);
  }
}

bool BattleStartBattleState::IsFinished() {
  return battleStartTimer.getElapsed().asMicroseconds() >= startupDelay + preBattleLength;
}