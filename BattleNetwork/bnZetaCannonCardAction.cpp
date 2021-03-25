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
  font(Font::Style::small), 
  timerLabel(font)
{ 
  timerLabel.SetFont(font);
  timerLabel.setPosition(20.f, 50.0f);
  timerLabel.setScale(1.f, 1.f);
  this->SetLockout({CardAction::LockoutType::async, timer});
}

ZetaCannonCardAction::~ZetaCannonCardAction()
{ }

void ZetaCannonCardAction::OnExecute() {
}

void ZetaCannonCardAction::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
  if (firstTime) return;

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
{ /*
  if (timer > 0) {
    auto user = GetOwner();
    auto actions = user->GetComponentsDerivedFrom<CardAction>();

    // TODO: some sort of event pipeline for actions needs to be restricted
    //       currently, each stacked action will override the current animation and will 
    //       prevent the action from ending correctly (if animation-based)
    //       Also, we do not want to have to allow the programmer to follow these following conditions:
    bool canShoot = user->GetFirstComponent<AnimationComponent>()->GetAnimationString() == "PLAYER_IDLE" && !user->IsSliding();

    if (canShoot && (firstTime || Input().Has(InputEvents::pressed_use_chip)) && actions.size() == 1) {
      auto attack = user->CreateComponent<CannonCardAction>(*user, CannonCardAction::Type::red, damage);

      auto actionProps = ActionLockoutProperties();
      actionProps.type = ActionLockoutType::animation;
      actionProps.group = ActionLockoutGroup::card;
      attack->SetLockout(actionProps);
      attack->OnExecute();

      if (firstTime) {
        firstTime = false;
        Audio().Play(AudioType::COUNTER_BONUS);
        defense = new DefenseIndestructable(true);
        user->AddDefenseRule(defense);
      }
    }
  }

  if (firstTime) return;

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

  CardAction::Update(_elapsed);*/
}

void ZetaCannonCardAction::OnAnimationEnd()
{
}

void ZetaCannonCardAction::OnEndAction()
{
  GetCharacter().RemoveDefenseRule(defense);
  delete defense;
}
