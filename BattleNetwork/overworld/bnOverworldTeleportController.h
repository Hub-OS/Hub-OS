#pragma once

#include <SFML/System.hpp>
#include <queue>
#include "bnOverworldSprite.h"
#include "../bnDirection.h"
#include "../bnAnimation.h"
#include "../bnCallback.h"
#include "../bnDrawWindow.h"
#include "../bnResourceHandle.h"

namespace Overworld {
  class Actor; // namespace Overworld::Actor;

  class TeleportController : public ResourceHandle {
  public:
    struct Command {
      enum class state {
        teleport_in = 0,
        teleport_out
      } const state{};
      const float originalWalkSpeed{};
      Callback<void()> onFinish;
    };

    TeleportController();
    ~TeleportController() = default;

    Command& TeleportOut(std::shared_ptr<Actor> actor);
    Command& TeleportIn(std::shared_ptr<Actor> actor, const sf::Vector3f& start, Direction dir, bool doSpin = false);
    void Update(double elapsed);
    bool TeleportedOut() const;
    const bool IsComplete() const;
    std::shared_ptr<WorldSprite> GetBeam();
    void EnableSound(bool enable);

  private:
    std::queue<Command> sequence;

    bool animComplete{ true }, walkoutComplete{ true }, entered{ false }, spin{ false }, teleportedOut{ false };
    bool mute{ false };
    float spinProgress{};
    frame_time_t walkFrames{};
    std::shared_ptr<Overworld::Actor> actor;
    std::shared_ptr<WorldSprite> beam;
    Animation beamAnim;
    Direction startDir;
  };
}