#include "bnOverworldPlayerController.h"
#include "../bnDirection.h"
#include "../bnInputManager.h"
#include "bnOverworldActor.h"

namespace {
  constexpr frame_time_t START_DELAY = frames(4);
  constexpr frame_time_t END_DELAY = frames(3);
}

void Overworld::PlayerController::ControlActor(std::shared_ptr<Actor> actor)
{
  this->actor = actor;
}

void Overworld::PlayerController::ReleaseActor()
{
  // stop moving
  if (this->actor) {
    this->actor->Face(actor->GetHeading());

    // set to nullptr
    this->actor = nullptr;

    frameDelay.reset();
    releaseFrameDelay.reset();
  }
}

void Overworld::PlayerController::Update(double elapsed)
{
  if (actor == nullptr) return;

  bool run = Input().Has(InputEvents::held_run);
  std::vector<Direction> inputs;

  if (listen) {
    auto prevDelay = frameDelay;

    if (Input().Has(InputEvents::pressed_move_left) || Input().Has(InputEvents::held_move_left)) {
      frameDelay.left += from_seconds(elapsed);
    }
    else if(Input().Has(InputEvents::released_move_left)) {
      releaseFrameDelay.left = ::END_DELAY;
    }

    if (Input().Has(InputEvents::pressed_move_right) || Input().Has(InputEvents::held_move_right)) {
      frameDelay.right += from_seconds(elapsed);
    }
    else if (Input().Has(InputEvents::released_move_right)) {
      releaseFrameDelay.right = ::END_DELAY;
    }

    if (Input().Has(InputEvents::pressed_move_up) || Input().Has(InputEvents::held_move_up)) {
      frameDelay.up += from_seconds(elapsed);
    }
    else if (Input().Has(InputEvents::released_move_up)) {
      releaseFrameDelay.up = ::END_DELAY;
    }

    if (Input().Has(InputEvents::pressed_move_down) || Input().Has(InputEvents::held_move_down)) {
      frameDelay.down += from_seconds(elapsed);
    }
    else if (Input().Has(InputEvents::released_move_down)) {
      releaseFrameDelay.down = ::END_DELAY;
    }

    bool directionEnded = false;

    // If one of the direction delays for release key hits EXACTLY ZERO, then
    // we terminate that direction and flag the boolean
    if (releaseFrameDelay.up > frames(0)) {
      releaseFrameDelay.up -= from_seconds(elapsed);

      if (releaseFrameDelay.up <= frames(0)) {
        frameDelay.up = frames(0);
        directionEnded = true;
      }
    }

    if (releaseFrameDelay.down > frames(0)) {
      releaseFrameDelay.down -= from_seconds(elapsed);

      if (releaseFrameDelay.down <= frames(0)) {
        frameDelay.down = frames(0);
        directionEnded = true;
      }
    }

    if (releaseFrameDelay.left > frames(0)) {
      releaseFrameDelay.left -= from_seconds(elapsed);

      if (releaseFrameDelay.left <= frames(0)) {
        frameDelay.left = frames(0);
        directionEnded = true;
      }
    }

    if (releaseFrameDelay.right > frames(0)) {
      releaseFrameDelay.right -= from_seconds(elapsed);

      if (releaseFrameDelay.right <= frames(0)) {
        frameDelay.right = frames(0);
        directionEnded = true;
      }
    }

    if (directionEnded) {
      // ONLY look for other directions < `END_DELAY` frames that are also ABOVE zero, otherwise
      // this will terminate directions that have not been released in the window of time and create
      // stiff, undesireable movement
      if (releaseFrameDelay.up > frames(0) && releaseFrameDelay.up <= ::END_DELAY) {
        frameDelay.up = frames(0);
      }

      if (releaseFrameDelay.down > frames(0) && releaseFrameDelay.down <= ::END_DELAY) {
        frameDelay.down = frames(0);
      }

      if (releaseFrameDelay.left > frames(0) && releaseFrameDelay.left <= ::END_DELAY) {
        frameDelay.left = frames(0);
      }

      if (releaseFrameDelay.right > frames(0) && releaseFrameDelay.right <= ::END_DELAY) {
        frameDelay.right = frames(0);
      }
    }

    if (frameDelay.left >= ::START_DELAY) {
      inputs.push_back(Direction::down_left);
    }

    if (frameDelay.right >= ::START_DELAY) {
      inputs.push_back(Direction::up_right);
    }

    if (frameDelay.up >= ::START_DELAY) {
      inputs.push_back(Direction::up_left);
    }

    if (frameDelay.down >= ::START_DELAY) {
      inputs.push_back(Direction::down_right);
    }
  }

  Direction dir = Direction::none;

  for (auto d : inputs) {
    dir = Join(dir, d);
  }

  if (dir != Direction::none) {
    if (run) {
      actor->Run(dir);
    }
    else {
      actor->Walk(dir);
    }
  }
  else {
    actor->Face(actor->GetHeading());
  }
}

void Overworld::PlayerController::ListenToInputEvents(const bool listen)
{
  this->listen = listen;

  if (this->listen == false) {
    frameDelay.reset();
    releaseFrameDelay.reset();
  }
}
