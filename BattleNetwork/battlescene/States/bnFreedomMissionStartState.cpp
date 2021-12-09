#include "bnFreedomMissionStartState.h"

#include "../../bnPlayer.h"
#include "../bnBattleSceneBase.h"
#include "../../bnAudioResourceManager.h"
#include "../../bnTextureResourceManager.h"
#include "../../bnPlayerSelectedCardsUI.h"

FreedomMissionStartState::FreedomMissionStartState(std::vector<std::shared_ptr<Player>>& tracked, uint8_t maxTurns) : 
  maxTurns(maxTurns),
  tracked(tracked) {
  battleStart = sf::Sprite(*Textures().LoadFromFile("resources/ui/counted_turn_start.png"));
  battleStart.setOrigin(battleStart.getLocalBounds().width / 2.0f, battleStart.getLocalBounds().height / 2.0f);
  battleStartPos = sf::Vector2f(240.f, 140.f);
  battleStart.setPosition(battleStartPos);
  battleStart.setScale(2.f, 2.f);
}

void FreedomMissionStartState::SetStartupDelay(frame_time_t frames)
{
  startupDelay = frames;
}

void FreedomMissionStartState::onStart(const BattleSceneState*)
{
  GetScene().IncrementTurnCount();

  if (maxTurns == GetScene().GetTurnCount()) {
    battleStart.setTexture(*Textures().LoadFromFile("resources/ui/final_turn_start.png"), true);
    battleStart.setOrigin(battleStart.getLocalBounds().width / 2.0f, battleStart.getLocalBounds().height / 2.0f);
  }

  battleStartTimer.reset();
  battleStartTimer.start();

  // only reveal first player's UI widget to them
  auto ui = GetScene().GetLocalPlayer()->GetFirstComponent<PlayerSelectedCardsUI>();

  if (ui) {
    ui->Reveal();
  }
}

void FreedomMissionStartState::onEnd(const BattleSceneState*)
{
}

void FreedomMissionStartState::onUpdate(double elapsed)
{
  battleStartTimer.update(sf::seconds(static_cast<float>(elapsed)));
}

void FreedomMissionStartState::onDraw(sf::RenderTexture& surface)
{
  frame_time_t start = from_milliseconds(battleStartTimer.getElapsed().asMilliseconds());

  if (start >= startupDelay) {
    double delta = (start - startupDelay).asSeconds().value;
    double length = preBattleLength.asSeconds().value;

    double scale = swoosh::ease::wideParabola(delta, length, 2.0);
    battleStart.setScale(2.f, static_cast<float>(scale * 2.0));

    surface.draw(GetScene().GetCardSelectWidget());
    surface.draw(battleStart);
  }
}

bool FreedomMissionStartState::IsFinished() {
  return battleStartTimer.getElapsed().asSeconds() >= (startupDelay + preBattleLength).asSeconds().value;
}