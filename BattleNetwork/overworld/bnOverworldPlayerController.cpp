#include "bnOverworldPlayerController.h"
#include "../bnDirection.h"
#include "../bnInputManager.h"
#include "bnOverworldActor.h"

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
  }
}

void Overworld::PlayerController::Update(double elapsed)
{
  if (actor == nullptr) return;

  bool run = Input().Has(InputEvents::held_run);
  std::vector<Direction> inputs;

  if (listen) {
    if (Input().Has(InputEvents::pressed_move_left) || Input().Has(InputEvents::held_move_left)) {
      frameDelay.left += from_seconds(elapsed);
    }
    else {
      frameDelay.left = frames(0);
    }

    if (Input().Has(InputEvents::pressed_move_right) || Input().Has(InputEvents::held_move_right)) {
      frameDelay.right += from_seconds(elapsed);
    }
    else {
      frameDelay.right = frames(0);
    }

    if (Input().Has(InputEvents::pressed_move_up) || Input().Has(InputEvents::held_move_up)) {
      frameDelay.up += from_seconds(elapsed);
    }
    else {
      frameDelay.up = frames(0);
    }

    if (Input().Has(InputEvents::pressed_move_down) || Input().Has(InputEvents::held_move_down)) {
      frameDelay.down += from_seconds(elapsed);
    }
    else {
      frameDelay.down = frames(0);
    }

    if (frameDelay.left >= frames(5)) {
      inputs.push_back(Direction::down_left);
    }

    if (frameDelay.right >= frames(5)) {
      inputs.push_back(Direction::up_right);
    }

    if (frameDelay.up >= frames(5)) {
      inputs.push_back(Direction::up_left);
    }

    if (frameDelay.down >= frames(5)) {
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
}
