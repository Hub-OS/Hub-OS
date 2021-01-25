#include "bnFalzarSpinState.h"
#include "bnFalzar.h"
#include "bnAnimationComponent.h"
#include "bnAudioResourceManager.h"

FalzarSpinState::FalzarSpinState(int spinCount) : spinCount(0), maxSpinCount(spinCount)
{
}

FalzarSpinState::~FalzarSpinState()
{
}

void FalzarSpinState::OnEnter(Falzar& falzar)
{
  spinCount = 0; // reset the spin count
  auto animation = falzar.GetFirstComponent<AnimationComponent>();
  animation->SetAnimation("SPIN");
  animation->SetPlaybackMode(Animator::Mode::Loop);
  
  // replace with drill sfx?
  falzar.Audio().Play(AudioType::INVISIBLE);
}

void FalzarSpinState::OnUpdate(double _elapsed, Falzar& falzar)
{
  // TODO: hacky slide/movement code on stream
  // improve movement API
  if (!falzar.IsSliding()) {
    falzar.SlideToTile(true);
    falzar.SetSlideTime(sf::seconds(speed));
    falzar.Move(Direction::left);
    falzar.FinishMove();

    if (!falzar.IsSliding()) {
      // replace with drill sfx?
      falzar.Audio().Play(AudioType::INVISIBLE);

      spinCount++;

      if (spinCount >= maxSpinCount) {
        falzar.Teleport(5, 2); // falzar re-appears in the middle
        falzar.AdoptNextTile();
        falzar.FinishMove();
        falzar.GoToNextState();
      }
      else {
        int row = (rand() % 3) + 1;
        falzar.Teleport(7, row);
        falzar.AdoptNextTile();
        falzar.FinishMove();
      }
    }
  }
}

void FalzarSpinState::OnLeave(Falzar&)
{
}
