#include "bnTimeFreezeBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnCard.h"
#include "../../bnCardAction.h"
#include "../../bnCharacter.h"

TimeFreezeBattleState::TimeFreezeBattleState() :
  font(Font::Style::thick)
{
  lockedTimestamp = lockedTimestamp = std::numeric_limits<long long>::max();
}

const bool TimeFreezeBattleState::FadeInBackdrop()
{
  return GetScene().FadeInBackdrop(backdropInc, 0.5, true);
}

const bool TimeFreezeBattleState::FadeOutBackdrop()
{
  return GetScene().FadeOutBackdrop(backdropInc);
}

void TimeFreezeBattleState::onStart(const BattleSceneState*)
{
  GetScene().GetSelectedCardsUI().Hide();
  GetScene().GetField()->ToggleTimeFreeze(true); // freeze everything on the field but accept hits
  summonTimer.reset();
  summonTimer.pause(); // if returning to this state, make sure the timer is not ticking at first
  currState = state::fadein;
}

void TimeFreezeBattleState::onEnd(const BattleSceneState*)
{
  GetScene().GetSelectedCardsUI().Reveal();
  GetScene().GetField()->ToggleTimeFreeze(false);
  GetScene().HighlightTiles(false);
  action = nullptr;
  user = nullptr;
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
      Audio().Play(AudioType::TIME_FREEZE, AudioPriority::highest);
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
      if (action && action->IsAnimationOver() == false) {
          action->Update(elapsed);
      }
      else{
        currState = state::fadeout;
        user->ToggleTimeFreeze(true); // in case the user was animating
        lockedTimestamp = std::numeric_limits<long long>::max();
      }
      GetScene().GetField()->Update(elapsed);
    }
    break;
  }
}

void TimeFreezeBattleState::onDraw(sf::RenderTexture& surface)
{
  Text summonsLabel = Text(name, font);

  double summonSecs = summonTimer.getElapsed().asSeconds();
  double scale = swoosh::ease::wideParabola(summonSecs, summonTextLength, 3.0);

  if (team == Team::red) {
    summonsLabel.setPosition(40.0f, 80.f);
  }
  else {
    summonsLabel.setPosition(470.0f, 80.0f);
  }

  summonsLabel.setScale(1.0f, (float)scale);
  summonsLabel.SetColor(sf::Color::White);

  if (team == Team::red) {
    summonsLabel.setOrigin(0, summonsLabel.GetLocalBounds().height);
  }
  else {
    summonsLabel.setOrigin(summonsLabel.GetLocalBounds().width, summonsLabel.GetLocalBounds().height);
  }

  surface.draw(GetScene().GetCardSelectWidget());
  surface.draw(summonsLabel);
}

void TimeFreezeBattleState::ExecuteTimeFreeze()
{
  auto actions = user->GetComponentsDerivedFrom<CardAction>();

  if (actions.empty()) return;

  action = actions[0];

  if (action && action->CanExecute()) {
    action->Execute();

    if (action->GetLockoutType() != ActionLockoutType::sequence) {
      user->ToggleTimeFreeze(false); // unfreeze the user to animate their sequences
    }
  }
}

bool TimeFreezeBattleState::IsOver() {
  return state::fadeout == currState && FadeOutBackdrop();
}

void TimeFreezeBattleState::OnCardUse(Battle::Card& card, Character& user, long long timestamp)
{
  if (card.IsTimeFreeze() && timestamp < lockedTimestamp) {
    this->name = card.GetShortName();
    this->team = user.GetTeam();
    this->user = &user;
    lockedTimestamp = timestamp;
  }
}
