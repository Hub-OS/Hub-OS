#include "bnTimeFreezeBattleState.h"
#include "../bnBattleSceneBase.h"
#include "../../bnCard.h"
#include "../../bnCardAction.h"
#include "../../bnCardToActions.h"
#include "../../bnCharacter.h"
#include "../../bnStuntDouble.h"
#include "../../bnField.h"
#include "../../bnPlayer.h"
#include "../../bnPlayerSelectedCardsUI.h"
TimeFreezeBattleState::TimeFreezeBattleState()
{
  lockedTimestamp = std::numeric_limits<long long>::max();
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
  if (tfEvents.size()) {
    auto iter = tfEvents.begin();

    if (iter->stuntDouble) {
      GetScene().GetField()->DeallocEntity(iter->stuntDouble->GetID());
    }

    // iter = tfEvents.erase(iter);
  }
}

std::shared_ptr<Character> TimeFreezeBattleState::CreateStuntDouble(std::shared_ptr<Character> from)
{
  CleanupStuntDouble();
  auto stuntDouble = std::make_shared<StuntDouble>(from);
  stuntDouble->Init();
  return stuntDouble;
}

void TimeFreezeBattleState::SkipToAnimateState()
{
  startState = state::animate;
  ExecuteTimeFreeze();
}

void TimeFreezeBattleState::SkipFrame()
{
  skipFrame = true;
}

void TimeFreezeBattleState::ProcessInputs()
{
  size_t player_idx = 0;
  for (auto& p : this->GetScene().GetAllPlayers()) {
    p->InputState().Process();

    if (p->InputState().Has(InputEvents::pressed_use_chip)) {
      Logger::Logf("InputEvents::pressed_use_chip for player %i", player_idx);
      auto cardsUI = p->GetFirstComponent<PlayerSelectedCardsUI>();
      if (cardsUI) {
        auto maybe_card = cardsUI->Peek();

        if (maybe_card.has_value()) {
          // convert meta data into a useable action object
          const Battle::Card& card = *maybe_card;

          if (CanCounter(p) && card.IsTimeFreeze()) {
            if (auto action = CardToAction(card, p, &GetScene().getController().CardPackageManager())) {
              action->SetMetaData(card.props); // associate the meta with this action object

              OnCardActionUsed(action, CurrentTime::AsMilli());

              cardsUI->DropNextCard();
            }
          }
        }

        p->GetChargeComponent().SetCharging(false);
      }
    }
    player_idx++;
  }
}

void TimeFreezeBattleState::onStart(const BattleSceneState*)
{
  Logger::Logf("TimeFreezeBattleState::onStart");
  GetScene().GetSelectedCardsUI().Hide();
  GetScene().GetField()->ToggleTimeFreeze(true); // freeze everything on the field but accept hits
  currState = startState;

  if (tfEvents.empty()) return;

  auto& first = tfEvents.begin();
  if (first->action && first->action->GetMetaData().skipTimeFreezeIntro) {
    SkipToAnimateState();
  }
}

void TimeFreezeBattleState::onEnd(const BattleSceneState*)
{
  Logger::Logf("TimeFreezeBattleState::onEnd");
  GetScene().GetSelectedCardsUI().Reveal();
  GetScene().GetField()->ToggleTimeFreeze(false);
  GetScene().HighlightTiles(false);
  tfEvents.clear();
  summonStart = false;
  summonTick = frames(0);
  startState = state::fadein; // assume fadein for most time freezes
}

void TimeFreezeBattleState::onUpdate(double elapsed)
{
  if (skipFrame) {
    skipFrame = false;
    return;
  }

  if (elapsed > 0.0) {
    GetScene().IncrementFrame();
  }

  ProcessInputs();

  if (summonStart) {
    summonTick += frames(1);
  }

  switch (currState) {
  case state::fadein:
  {
    if (FadeInBackdrop()) {
      summonStart = true;
      currState = state::display_name;
      Audio().Play(AudioType::TIME_FREEZE, AudioPriority::highest);
    }
  }
    break;
  case state::display_name:
  {
    if (playerCountered) {
      playerCountered = false;

      Audio().Play(AudioType::TRAP, AudioPriority::high);
      auto& last = tfEvents.begin()+1u;
      last->animateCounter = true;
      summonTick = frames(0);
    }

    if (summonTick >= summonTextLength) {
      GetScene().HighlightTiles(true); // re-enable tile highlighting for new entities
      currState = state::animate; // animate this attack

      // Resize the time freeze queue to a max of 2 attacks
      tfEvents.resize(std::min(tfEvents.size(), 2ull));
      tfEvents.shrink_to_fit();

      ExecuteTimeFreeze();
    }

    for (auto& e : tfEvents) {
      if (e.animateCounter) {
        e.alertFrameCount += frames(1);
      }
    }
  }
  break;
  case state::animate:
    {
      bool updateAnim = false;
      auto& first = tfEvents.begin();

      if (first->action) {
        // update the action until it is is complete
        switch (first->action->GetLockoutType()) {
        case CardAction::LockoutType::sequence:
          updateAnim = !first->action->IsLockoutOver();
          break;
        default:
          updateAnim = !first->action->IsAnimationOver();
          break;
        }
      }

      if (updateAnim) {
        first->action->Update(elapsed);
      }
      else{
        first->user->Reveal();
        GetScene().GetField()->DeallocEntity(first->stuntDouble->GetID());

        if (tfEvents.size() == 1) {
          // This is the only event in the list
          lockedTimestamp = std::numeric_limits<long long>::max();
          currState = state::fadeout;
        }
        else {
          tfEvents.erase(tfEvents.begin());

          // restart the time freeze animation
          currState = state::animate;

          ExecuteTimeFreeze();
        }
      }
    }
    break;
  }
  GetScene().GetField()->Update(elapsed);
}

void TimeFreezeBattleState::onDraw(sf::RenderTexture& surface)
{
  if (tfEvents.empty()) return;

  auto& first = tfEvents.begin();
  static Text summonsLabel = Text(first->name, Font::Style::thick);
  static sf::Sprite alertSprite(*Textures().LoadFromFile("resources/ui/alert.png"));

  double scale = swoosh::ease::linear(summonTick.asSeconds().value, fadeInOutLength.asSeconds().value, 1.0);
  scale = std::min(scale, 1.0);

  if (summonTick >= summonTextLength - fadeInOutLength) {
    scale = swoosh::ease::linear((summonTextLength - summonTick).asSeconds().value, fadeInOutLength.asSeconds().value, 1.0);
    scale = std::max(scale, 0.0);
  }

  sf::Vector2f position = sf::Vector2f(66.f, 82.f);

  if (first->team == Team::blue) {
    position = sf::Vector2f(416.f, 82.f);
  }

  summonsLabel.setScale(2.0f, 2.0f*(float)scale);

  if (first->team == Team::red) {
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

  // draw the !! sprite
  for (auto& e : tfEvents) {
    if (e.animateCounter) {
      double scale = swoosh::ease::wideParabola(e.alertFrameCount.asSeconds().value, this->alertAnimFrames.asSeconds().value, 3.0);
      
      sf::Vector2f position = sf::Vector2f(66.f, 82.f);

      if (e.team == Team::blue) {
        position = sf::Vector2f(416.f, 82.f);
      }

      alertSprite.setScale(2.0f, 2.0f * (float)scale);

      if (e.team == Team::red) {
        alertSprite.setOrigin(0, alertSprite.getLocalBounds().height * 0.5f);
      }
      else {
        alertSprite.setOrigin(alertSprite.getLocalBounds().width, alertSprite.getLocalBounds().height * 0.5f);
      }

      alertSprite.setPosition(position);
      surface.draw(alertSprite);
    }
  }
}

void TimeFreezeBattleState::ExecuteTimeFreeze()
{
  if (tfEvents.empty()) return;

  auto& first = tfEvents.begin();

  if (first->action && first->action->CanExecute()) {
    first->user->Hide();
    if (GetScene().GetField()->AddEntity(first->stuntDouble, *first->user->GetTile()) != Field::AddEntityStatus::deleted) {
      first->action->Execute(first->user);
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
  Logger::Logf("OnCardActionUsed(): %s, summonTick: %i, summonTextLength: %i", action->GetMetaData().shortname.c_str(), summonTick.count(), summonTextLength.count());

  if (!(action && CanCounter(action->GetActor()) && action->GetMetaData().timeFreeze)) return;
 
  HandleTimeFreezeCounter(action, timestamp);
}

const bool TimeFreezeBattleState::CanCounter(std::shared_ptr<Character> user)
{
  // tfc window ended
  if (summonTick > summonTextLength) return false;

  bool addEvent = true;

  if (!tfEvents.empty()) {
    // only opposing players can counter
    auto lastActor = tfEvents.begin()->action->GetActor();
    if (!lastActor->Teammate(user->GetTeam())) {
      playerCountered = true;
      Logger::Logf("Player was countered!");
    }
    else {
      addEvent = false;
    }
  }

  return addEvent;
}

void TimeFreezeBattleState::HandleTimeFreezeCounter(std::shared_ptr<CardAction> action, uint64_t timestamp)
{
  TimeFreezeBattleState::EventData data;
  data.action = action;
  data.name = action->GetMetaData().shortname;
  data.team = action->GetActor()->GetTeam();
  data.user = action->GetActor();
  lockedTimestamp = timestamp;

  data.action = action;
  data.stuntDouble = CreateStuntDouble(data.user);
  action->UseStuntDouble(data.stuntDouble);

  tfEvents.insert(tfEvents.begin(), data);

  Logger::Logf("Added chip event: %s", data.name.c_str());
}

