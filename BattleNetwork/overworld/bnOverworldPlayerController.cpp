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
    if (Input().Has(InputEvents::held_move_left)) {
      inputs.push_back(Direction::down_left);
    }

    if (Input().Has(InputEvents::held_move_right)) {
      inputs.push_back(Direction::up_right);
    }

    if (Input().Has(InputEvents::held_move_up)) {
      inputs.push_back(Direction::up_left);
    }

    if (Input().Has(InputEvents::held_move_down)) {
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
