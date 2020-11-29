#include "bnOverworldPlayerController.h"
#include "../bnDirection.h"
#include "../bnInputManager.h"
#include "bnOverworldActor.h"

void Overworld::PlayerController::ControlActor(Actor& actor)
{
  this->actor = &actor;
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

  bool run = INPUTx.Has(EventTypes::HELD_CANCEL);
  std::vector<Direction> inputs;

  if (listen) {
    if (INPUTx.Has(EventTypes::PRESSED_MOVE_LEFT) || INPUTx.Has(EventTypes::HELD_MOVE_LEFT)) {
      inputs.push_back(Direction::down_left);
    }

    if (INPUTx.Has(EventTypes::PRESSED_MOVE_RIGHT) || INPUTx.Has(EventTypes::HELD_MOVE_RIGHT)) {
      inputs.push_back(Direction::up_right);
    }

    if (INPUTx.Has(EventTypes::PRESSED_MOVE_UP) || INPUTx.Has(EventTypes::HELD_MOVE_UP)) {
      inputs.push_back(Direction::up_left);
    }

    if (INPUTx.Has(EventTypes::PRESSED_MOVE_DOWN) || INPUTx.Has(EventTypes::HELD_MOVE_DOWN)) {
      inputs.push_back(Direction::down_right);
    }

    if (INPUTx.Has(EventTypes::PRESSED_CONFIRM)) {
      Direction facing = actor->GetHeading();

      for (auto other : actor->GetQuadTree()->GetActors()) {
        if (actor == other) continue;

        auto& [hit, _] = actor->CollidesWith(*other, Actor::MakeVectorFromDirection(facing, 5.0f));
        if (hit) {
          actor->Face(facing);
          other->Interact(*this->actor);
          return;
        }
      }
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
