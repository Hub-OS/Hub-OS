#include "bnZetaCannonCardAction.h"
#include "bnCannonCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDefenseIndestructable.h"
#include "bnInputManager.h"
#include "bnCharacter.h"
#include "bnDefenseRule.h"

ZetaCannonCardAction::ZetaCannonCardAction(std::shared_ptr<Character> actor, int damage)  : 
  CardAction(actor, "PLAYER_IDLE"), 
  InputHandle(),
  damage(damage), 
  font(Font::Style::thick), 
  timerLabel(font)
{ 
  timerLabel.SetFont(font);
  timerLabel.setPosition(40.f, 70.0f);
  timerLabel.setScale(2.f, 2.f);
  this->SetLockout({CardAction::LockoutType::async, timer});
}

ZetaCannonCardAction::~ZetaCannonCardAction()
{ 
  GetActor()->RemoveDefenseRule(defense);
}

void ZetaCannonCardAction::OnExecute(std::shared_ptr<Character>) {
}

void ZetaCannonCardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  if (!showTimerText || hide) return;

  auto pos = timerLabel.getPosition();
  auto og = pos;
  timerLabel.setPosition(pos.x + 2, pos.y + 2);
  timerLabel.SetColor(sf::Color::Black);
  target.draw(timerLabel);
  timerLabel.setPosition(og);
  timerLabel.SetColor(sf::Color::White);
  target.draw(timerLabel);
}

void ZetaCannonCardAction::Update(double _elapsed)
{
  if (timer > 0) {
    auto actor = this->GetActor();

    // TODO: change the "PLAYER_IDLE" check to a `IsActionable()` function per mars' notes...
    bool canShoot = actor->GetFirstComponent<AnimationComponent>()->GetAnimationString() == "PLAYER_IDLE" && !actor->IsMoving();

    if (canShoot && Input().Has(InputEvents::pressed_use_chip)) {

      auto newCannon = std::make_shared<CannonCardAction>(actor, CannonCardAction::Type::red, damage);
      auto actionProps = CardAction::LockoutProperties();
      actionProps.type = CardAction::LockoutType::animation;
      actionProps.group = CardAction::LockoutGroup::card;
      newCannon->SetLockout(actionProps);

      auto event = CardEvent{ std::shared_ptr<CardAction>(newCannon) };
      actor->AddAction(event, ActionOrder::voluntary);

      if (!showTimerText) {
        showTimerText = true;
        Audio().Play(AudioType::COUNTER_BONUS);
        defense = std::make_shared<DefenseIndestructable>(true);
        actor->AddDefenseRule(defense);

        if (actor->GetTeam() == Team::blue) {
          hide = true;
        }
      }
    }
  }

  if (showTimerText) {
    timer = std::max(0.0, timer - _elapsed);

    // Create an output string stream
    std::ostringstream sstream;
    sstream << std::fixed;
    sstream << std::setprecision(2);
    sstream << timer;

    // Get string from output string stream
    std::string timeString = sstream.str();

    // Set timer
    std::string string = "Z-Cannon 1: " + timeString;
    timerLabel.SetString(string);
  }

  CardAction::Update(_elapsed);
}

void ZetaCannonCardAction::OnAnimationEnd()
{
}

void ZetaCannonCardAction::OnActionEnd()
{ }
