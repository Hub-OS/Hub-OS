#include "bnTimeFreezeBattleState.h"

#include "../bnBattleSceneBase.h"

const bool TimeFreezeBattleState::FadeInBackdrop()
{
  return GetScene().FadeInBackdrop(0.1, 0.5, false);
}

const bool TimeFreezeBattleState::FadeOutBackdrop()
{
  return GetScene().FadeOutBackdrop(0.1);
}

void TimeFreezeBattleState::onStart()
{
  currState = state::fadein;
  GetScene().GetField()->ToggleTimeFreeze(true);
}

void TimeFreezeBattleState::onEnd()
{
  GetScene().GetField()->ToggleTimeFreeze(false);
}

void TimeFreezeBattleState::onUpdate(double elapsed)
{
  GetScene().GetField()->Update((float)elapsed);

  switch (currState) {
  case state::fadein:
    FadeInBackdrop() ? currState = state::animate : (void)0;
    break;
  case state::animate:
    // start the chip
    break;
  }

  // TODO: when chip is over, switch to the fadeout
}

void TimeFreezeBattleState::onDraw(sf::RenderTexture& surface)
{
  sf::Text summonsLabel = sf::Text("TEXT HERE", font);

  double summonSecs = summonTimer.getElapsed().asSeconds() - showSummonBackdropLength;
  double scale = swoosh::ease::wideParabola(summonSecs, summonTextLength, 3.0);

  Team team = Team::red;
  if (team == Team::red) {
    summonsLabel.setPosition(40.0f, 80.f);
  }
  else {
    summonsLabel.setPosition(470.0f, 80.0f);
  }

  summonsLabel.setScale(1.0f, (float)scale);
  summonsLabel.setOutlineColor(sf::Color::Black);
  summonsLabel.setFillColor(sf::Color::White);
  summonsLabel.setOutlineThickness(2.f);

  if (team == Team::red) {
    summonsLabel.setOrigin(0, summonsLabel.getLocalBounds().height);
  }
  else {
    summonsLabel.setOrigin(summonsLabel.getLocalBounds().width, summonsLabel.getLocalBounds().height);
  }

  ENGINE.Draw(summonsLabel, false);

  if (summonSecs >= summonTextLength) {
    ExecuteSummons();
    showSummonText = false;
  }
}

void TimeFreezeBattleState::ExecuteSummons()
{
}

bool TimeFreezeBattleState::IsOver() {
  return state::fadeout == currState && FadeOutBackdrop();
}
