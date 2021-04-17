#include "bnZetaCannonCardAction.h"
#include "bnCannonCardAction.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnDefenseIndestructable.h"
#include "bnInputManager.h"

ZetaCannonCardAction::ZetaCannonCardAction(Character& owner, int damage)  : 
  CardAction(owner, "PLAYER_IDLE"), 
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
  GetCharacter().RemoveDefenseRule(defense);
  delete defense;
}

void ZetaCannonCardAction::OnExecute() {
}

void ZetaCannonCardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  if (!showTimerText) return;

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
    auto& user = this->GetCharacter();

    // TODO: change the "PLAYER_IDLE" check to a `IsActionable()` function per mars' notes...
    bool canShoot = user.GetFirstComponent<AnimationComponent>()->GetAnimationString() == "PLAYER_IDLE" && !user.IsMoving();

    if (canShoot && Input().Has(InputEvents::pressed_use_chip)) {

      auto* newCannon = new CannonCardAction(user, CannonCardAction::Type::red, damage);
      auto actionProps = CardAction::LockoutProperties();
      actionProps.type = CardAction::LockoutType::animation;
      actionProps.group = CardAction::LockoutGroup::card;
      newCannon->SetLockout(actionProps);

      auto event = CardEvent{ newCannon };
      user.AddAction(event, ActionOrder::voluntary);

      if (!showTimerText) {
        showTimerText = true;
        Audio().Play(AudioType::COUNTER_BONUS);
        defense = new DefenseIndestructable(true);
        user.AddDefenseRule(defense);
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

void ZetaCannonCardAction::OnEndAction()
{ }
