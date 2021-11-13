#include "bnTimeFreezeBattleState.h"

#include "../bnBattleSceneBase.h"
#include "../../bnCard.h"
#include "../../bnCardAction.h"
#include "../../bnCardToActions.h"
#include "../../bnCharacter.h"
#include "../../bnStuntDouble.h"
#include "../../bnField.h"

TimeFreezeBattleState::TimeFreezeBattleState()
{
  lockedTimestamp = lockedTimestamp = std::numeric_limits<long long>::max();
}

TimeFreezeBattleState::~TimeFreezeBattleState()
{
}

const bool TimeFreezeBattleState::FadeInBackdrop()
{
  return GetScene().FadeInBackdrop(backdropInc, 0.5, true);
}

const bool TimeFreezeBattleState::FadeOutBackdrop()
{
  return GetScene().FadeOutBackdrop(backdropInc);
}

void TimeFreezeBattleState::CleanupStuntDouble()
{
  if (stuntDouble) {
    GetScene().GetField()->DeallocEntity(stuntDouble->GetID());
  }

  stuntDouble = nullptr;
}

std::shared_ptr<Character> TimeFreezeBattleState::CreateStuntDouble(std::shared_ptr<Character> from)
{
  CleanupStuntDouble();
  stuntDouble = std::make_shared<StuntDouble>(from);
  stuntDouble->Init();
  return stuntDouble;
}

void TimeFreezeBattleState::SkipToAnimateState()
{
  startState = state::animate;
  ExecuteTimeFreeze();
}

void TimeFreezeBattleState::onStart(const BattleSceneState*)
{
  GetScene().GetSelectedCardsUI().Hide();
  GetScene().GetField()->ToggleTimeFreeze(true); // freeze everything on the field but accept hits
  summonTimer.reset();
  summonTimer.pause(); // if returning to this state, make sure the timer is not ticking at first
  currState = startState;

  if (action && action->GetMetaData().skipTimeFreezeIntro) {
    SkipToAnimateState();
  }
}

void TimeFreezeBattleState::onEnd(const BattleSceneState*)
{
  GetScene().GetSelectedCardsUI().Reveal();
  GetScene().GetField()->ToggleTimeFreeze(false);
  GetScene().HighlightTiles(false);
  action = nullptr;
  user = nullptr;
  startState = state::fadein; // assume fadein for most time freezes
  executeOnce = false; // reset execute flag
}

void TimeFreezeBattleState::onUpdate(double elapsed)
{
  summonTimer.update(sf::seconds(static_cast<float>(elapsed)));

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
      currState = state::animate; // animate this attack
      ExecuteTimeFreeze();
    }
  }
  break;
  case state::animate:
    {
      bool updateAnim = false;

      if (action) {
        // update the action until it is is complete
        switch (action->GetLockoutType()) {
        case CardAction::LockoutType::sequence:
          updateAnim = !action->IsLockoutOver();
          break;
        default:
          updateAnim = !action->IsAnimationOver();
          break;
        }
      }

      if (updateAnim) {
        // For all new input events, set the wait time based on the network latency and append
        const auto events_this_frame = Input().StateThisFrame();

        std::vector<InputEvent> outEvents;
        for (auto& [name, state] : events_this_frame) {
          InputEvent copy;
          copy.name = name;
          copy.state = state;

          outEvents.push_back(copy);

          // add delay for network
          // copy.wait = from_seconds(currLag / 1000.0); // note: this is dumb. casting to seconds just to cast back to milli internally...
          //inputQueue.push_back(copy);
          user->InputState().VirtualKeyEvent(copy);
        }
        user->InputState().Process();
        // user->InputState().DebugPrint();

        action->Update(elapsed);
      }
      else{
        currState = state::fadeout;
        user->Reveal();
        CleanupStuntDouble();
        lockedTimestamp = std::numeric_limits<long long>::max();
      }
    }
    break;
  }
  GetScene().GetField()->Update(elapsed);
}

void TimeFreezeBattleState::onDraw(sf::RenderTexture& surface)
{
  Text summonsLabel = Text(name, Font::Style::thick);

  double summonSecs = summonTimer.getElapsed().asSeconds();
  double scale = swoosh::ease::wideParabola(summonSecs, summonTextLength, 3.0);

  sf::Vector2f position = sf::Vector2f(66.f, 82.f);

  if (team == Team::blue) {
    position = sf::Vector2f(416.f, 82.f);
  }

  summonsLabel.setScale(2.0f, 2.0f*(float)scale);

  if (team == Team::red) {
    summonsLabel.setOrigin(0, summonsLabel.GetLocalBounds().height*0.5f);
  }
  else {
    summonsLabel.setOrigin(summonsLabel.GetLocalBounds().width, summonsLabel.GetLocalBounds().height*0.5f);
  }

  surface.draw(GetScene().GetCardSelectWidget());

  summonsLabel.SetColor(sf::Color::Black);
  summonsLabel.setPosition(position.x + 2.f, position.y + 2.f);
  surface.draw(summonsLabel);
  summonsLabel.SetColor(sf::Color::White);
  summonsLabel.setPosition(position);
  surface.draw(summonsLabel);
}

void TimeFreezeBattleState::ExecuteTimeFreeze()
{
  if (action && action->CanExecute()) {
    user->Hide();
    if (GetScene().GetField()->AddEntity(stuntDouble, *user->GetTile()) != Field::AddEntityStatus::deleted) {
      action->Execute(user);
    }
    else {
      currState = state::fadeout;
    }
  }
}

bool TimeFreezeBattleState::IsOver() {
  return state::fadeout == currState && FadeOutBackdrop();
}

void TimeFreezeBattleState::OnCardActionUsed(std::shared_ptr<CardAction> action, uint64_t timestamp)
{
  if (!action)
    return;

  if (action->GetMetaData().timeFreeze && timestamp < lockedTimestamp) {
    this->name = action->GetMetaData().shortname;
    this->team = action->GetActor()->GetTeam();
    this->user = action->GetActor();
    lockedTimestamp = timestamp;

    this->action = action;
    stuntDouble = CreateStuntDouble(this->user);
    action->UseStuntDouble(stuntDouble);
  }
}