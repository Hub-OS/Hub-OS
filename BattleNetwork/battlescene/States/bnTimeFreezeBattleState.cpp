#include "bnTimeFreezeBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnCard.h"
#include "../../bnCardAction.h"
#include "../../bnCharacter.h"

const bool TimeFreezeBattleState::FadeInBackdrop()
{
  return GetScene().FadeInBackdrop(backdropInc, 0.5, true);
}

const bool TimeFreezeBattleState::FadeOutBackdrop()
{
  return GetScene().FadeOutBackdrop(backdropInc);
}

TimeFreezeBattleState::TimeFreezeBattleState()
{
  font = TEXTURES.LoadFontFromFile("resources/fonts/mmbnthick_regular.ttf");
}

void TimeFreezeBattleState::onStart(const BattleSceneState*)
{

  GetScene().GetSelectedCardsUI().Hide();
  GetScene().GetField()->ToggleTimeFreeze(true); // freeze everything on the field but accept hits
  summonTimer.reset();
  summonTimer.pause(); // if returning to this state, make sure the timer is not ticking at first
  currState = state::fadein;
  action = nullptr;
}

void TimeFreezeBattleState::onEnd(const BattleSceneState*)
{
  GetScene().GetSelectedCardsUI().Reveal();
  GetScene().GetField()->ToggleTimeFreeze(false);
  GetScene().HighlightTiles(false);
}

void TimeFreezeBattleState::onUpdate(double elapsed)
{
  summonTimer.update(elapsed);

  switch (currState) {
  case state::fadein:
  {
    if (FadeInBackdrop()) {
      currState = state::display_name;
      summonTimer.start();
      AUDIO.Play(AudioType::TIME_FREEZE, AudioPriority::highest);
    }
  }
    break;
  case state::display_name:
  {
    double summonSecs = summonTimer.getElapsed().asSeconds();

    if (summonSecs >= summonTextLength) {
      GetScene().HighlightTiles(true); // re-enable tile highlighting for new entities

      ExecuteTimeFreeze();
      currState = state::animate; // animate this attack
    }
  }
  break;
  case state::animate:
    {
      GetScene().GetField()->Update(elapsed);
      if (action && action->IsAnimationOver() == false) {
          action->Update(elapsed);
      }
      else{
        currState = state::fadeout;
      }
    }
    break;
  }
}

void TimeFreezeBattleState::onDraw(sf::RenderTexture& surface)
{
  sf::Text summonsLabel = sf::Text(sf::String(name), *font);

  double summonSecs = summonTimer.getElapsed().asSeconds();
  double scale = swoosh::ease::wideParabola(summonSecs, summonTextLength, 3.0);

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

  ENGINE.Draw(GetScene().GetCardSelectWidget());
  ENGINE.Draw(summonsLabel, false);
}

void TimeFreezeBattleState::ExecuteTimeFreeze()
{
  // start the chip
  action = user->GetFirstComponent<CardAction>();
  action ? action->OnExecute() : (void)0;
}

bool TimeFreezeBattleState::IsOver() {
  return state::fadeout == currState && FadeOutBackdrop();
}

void TimeFreezeBattleState::OnCardUse(Battle::Card& card, Character& user, long long timestamp)
{
  this->name = card.GetShortName();
  this->team = user.GetTeam();
  this->user = &user;
}
