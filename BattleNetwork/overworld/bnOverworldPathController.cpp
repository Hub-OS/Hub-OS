#include "bnOverworldPathController.h"
#include "bnOverworldActor.h"

void Overworld::PathController::ControlActor(Actor& actor)
{
  this->actor = &actor;
}

void Overworld::PathController::Update(double elapsed)
{
  if (actions.isEmpty()) {
    for (auto& command : commands) {
      command();
    }
  }

  actions.update(elapsed);
}

void Overworld::PathController::AddPoint(const sf::Vector2f& point)
{
  auto closure = [=] {
    actions.add(new MoveToCommand(this->actor, point));
  };

  commands.push_back(closure);
}

void Overworld::PathController::AddWait(const frame_time_t& frames)
{
  auto closure = [=] {
    actions.add(new WaitCommand(this->actor, frames));
  };

  commands.push_back(closure);
}

void Overworld::PathController::ClearPoints() {
  actions.clear();
  commands.clear();
}

// class PathController::MoveToCommand

Overworld::PathController::MoveToCommand::MoveToCommand(Overworld::Actor* actor, const sf::Vector2f& dest) :
  actor(actor),
  dest(dest),
  swoosh::BlockingActionItem()
{

}

void Overworld::PathController::MoveToCommand::update(double elapsed)
{
  if (actor) {
    auto vec = dest - actor->getPosition();
    float mag = std::sqrtf((vec.x * vec.x) + (vec.y * vec.y));

    if (mag > 0.001f) {
      actor->Walk(Actor::MakeDirectionFromVector(vec, 0.01f));
    }
    else {
      markDone();
    }
  }
}

void Overworld::PathController::MoveToCommand::draw(sf::RenderTexture& surface)
{
  // none
}

// class PathController::WaitCommand

Overworld::PathController::WaitCommand::WaitCommand(Overworld::Actor* actor, const frame_time_t& frames) :
  actor(actor),
  frames(frames),
  swoosh::BlockingActionItem()
{
}

void Overworld::PathController::WaitCommand::update(double elapsed)
{
  frames -= frame_time_t::from_seconds(elapsed);

  if (frames <= ::frames(0)) {
    markDone();
  }

  actor->Face(actor->GetHeading());
}

void Overworld::PathController::WaitCommand::draw(sf::RenderTexture& surface)
{
  // none
}
